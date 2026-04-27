/*
 * ============================================================
 *  SMART FLOOD PREVENTION PUMP SYSTEM
 *  Self-Tuning PID Water Pump Controller - Wokwi Simulation
 *  Platform  : ESP32 DevKit V1
 *  Author    : Sistem Otomasi - Wokwi Sim
 * ============================================================
 *
 *  KOMPONEN YANG DIGUNAKAN:
 *  - ESP32 DevKit V1          → Microcontroller utama
 *  - Potensiometer            → Simulasi sensor level air
 *  - L298N / Motor Driver     → Driver pompa motor DC
 *  - DC Motor                 → Simulasi pompa air
 *  - LED Hijau / Kuning / Merah → Indikator status
 *  - Buzzer                   → Alarm bahaya banjir
 *  - LCD I2C 16x2             → Display informasi lokal
 *
 *  CARA PAKAI:
 *  1. Putar potensiometer untuk simulasikan kenaikan level air
 *  2. Amati motor berputar (pompa aktif) sesuai level air
 *  3. Ketik 'T' di Serial Monitor → mulai Auto-Tune PID
 *  4. Ketik 'S' → lihat status lengkap
 *  5. Ketik 'H' → lihat semua perintah
 *
 *  LOGIKA LEVEL AIR:
 *  < 30%  → AMAN    → LED Hijau  → Pompa OFF
 *  30-50% → WASPADA → LED Kuning → Pompa pelan
 *  50-70% → SIAGA   → LED Merah+Kuning → Pompa sedang
 *  > 70%  → BAHAYA  → LED Merah  → Pompa maksimum + ALARM
 * ============================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ─── PIN DEFINITIONS ─────────────────────────────────────────
#define POT_PIN         34   // Potensiometer (ADC1_CH6)
#define MOTOR_ENA       27   // PWM Enable Motor (L298N)
#define MOTOR_IN1       25   // Arah Motor 1
#define MOTOR_IN2       26   // Arah Motor 2
#define LED_GREEN       32   // LED Hijau - Aman
#define LED_YELLOW      33   // LED Kuning - Waspada
#define LED_RED         4    // LED Merah - Bahaya
#define BUZZER_PIN      2    // Buzzer Alarm
#define SDA_PIN         21   // I2C SDA
#define SCL_PIN         22   // I2C SCL

// ─── PWM MOTOR (ESP32 LEDC API) ──────────────────────────────
#define MOTOR_FREQ      5000
#define MOTOR_RES       8    // 8-bit: 0-255

// ─── BATAS LEVEL AIR (%) ─────────────────────────────────────
#define LVL_SAFE        30.0
#define LVL_WARNING     50.0
#define LVL_DANGER      70.0
#define SETPOINT        25.0   // Target level air yg ingin dipertahankan

// ─── PID PARAMETERS (default, bisa di-autotune) ──────────────
double Kp = 2.5;
double Ki = 0.8;
double Kd = 0.3;

// ─── PID STATE ───────────────────────────────────────────────
double pidInput    = 0.0;
double pidOutput   = 0.0;
double pidSetpoint = SETPOINT;
double errSum      = 0.0;
double lastErr     = 0.0;
double currentErr  = 0.0;

// ─── AUTO-TUNE STATE (Relay Method) ──────────────────────────
bool   atActive    = false;  // autotune sedang berjalan
bool   atDone      = false;
double atRelay     = 50.0;   // amplitudo relay output
double atMax       = -999.0;
double atMin       =  999.0;
int    atCrossings = 0;
bool   atLastAbove = false;
unsigned long atLastCross = 0;
unsigned long atPeriodSum = 0;
unsigned long atStart     = 0;
double Ku = 0, Tu = 0;

// ─── TIMING ──────────────────────────────────────────────────
unsigned long tPID   = 0;
unsigned long tPrint = 0;
unsigned long tBuzz  = 0;
const int DT_PID    = 100;   // ms, interval PID
const int DT_PRINT  = 600;   // ms, interval serial output

// ─── OUTPUT ──────────────────────────────────────────────────
int    motorPWM  = 0;
int    pumpPct   = 0;
bool   alarmOn   = false;
String statusStr = "AMAN";

// ─── LCD ─────────────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Custom chars
byte charWave[8] = {0b00000,0b00000,0b01010,0b10101,0b10001,0b01010,0b00000,0b00000};
byte charPump[8] = {0b00100,0b01110,0b11111,0b01110,0b00100,0b11111,0b01110,0b00100};
byte charUp[8]   = {0b00100,0b01110,0b11111,0b00100,0b00100,0b00100,0b00000,0b00000};

// ═════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(300);

  // LCD init
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, charWave);
  lcd.createChar(1, charPump);
  lcd.createChar(2, charUp);

  // Splash
  lcd.setCursor(2, 0); lcd.print("FLOOD CONTROL");
  lcd.setCursor(3, 1); lcd.print("PID SYSTEM");
  delay(2000);
  lcd.clear();

  // Pin modes
  pinMode(LED_GREEN,  OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED,    OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MOTOR_IN1,  OUTPUT);
  pinMode(MOTOR_IN2,  OUTPUT);
  analogReadResolution(12);  // 0-4095

  // Motor forward direction
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);

  // Motor PWM setup (ESP32 core 3.x API)
  ledcAttach(MOTOR_ENA, MOTOR_FREQ, MOTOR_RES);
  ledcWrite(MOTOR_ENA, 0);

  // LED test
  for (int pin : {LED_GREEN, LED_YELLOW, LED_RED}) {
    digitalWrite(pin, HIGH); delay(200); digitalWrite(pin, LOW);
  }
  digitalWrite(BUZZER_PIN, HIGH); delay(80); digitalWrite(BUZZER_PIN, LOW);

  printHelp();
  tPID   = millis();
  tPrint = millis();
}

// ═════════════════════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  handleSerial();
  pidInput = readLevel();

  if (now - tPID >= DT_PID) {
    tPID = now;
    if (atActive) runAutoTune(now);
    else           runPID();
    setMotor(motorPWM);
    updateLEDs();
    updateLCD();
  }

  handleBuzzer(now);

  if (now - tPrint >= DT_PRINT) {
    tPrint = now;
    printSerial(now / 1000);
  }
}

// ─────────────────────────────────────────────────────────────
//  BACA LEVEL AIR (via Potensiometer)
//  0% = sungai kosong, 100% = sungai meluap
// ─────────────────────────────────────────────────────────────
double readLevel() {
  int raw = analogRead(POT_PIN);
  return constrain((raw / 4095.0) * 100.0, 0.0, 100.0);
}

// ─────────────────────────────────────────────────────────────
//  PID CONTROLLER
//  Error positif = level air > setpoint → pompa harus kerja
// ─────────────────────────────────────────────────────────────
void runPID() {
  double dt  = DT_PID / 1000.0;  // konversi ms → detik
  currentErr = pidInput - pidSetpoint;

  // Jika air sudah di bawah setpoint → pompa tidak perlu kerja
  if (currentErr <= 0.0) {
    errSum  = 0.0;
    motorPWM = 0;
    pidOutput = 0;
    lastErr   = currentErr;
    return;
  }

  errSum += currentErr * dt;
  errSum  = constrain(errSum, -200.0, 200.0);  // anti-windup

  double derivative = (currentErr - lastErr) / dt;
  pidOutput = (Kp * currentErr) + (Ki * errSum) + (Kd * derivative);

  motorPWM = (int)constrain(pidOutput, 0.0, 255.0);
  lastErr  = currentErr;
}

// ─────────────────────────────────────────────────────────────
//  AUTO-TUNE — Relay (Åström-Hägglund) Method
//  Prinsip: berikan output relay (ON/OFF) di sekitar setpoint
//  Biarkan sistem berosilasi → ukur Ku dan Tu
//  Hitung Kp, Ki, Kd via Ziegler-Nichols
// ─────────────────────────────────────────────────────────────
void runAutoTune(unsigned long now) {
  bool above = (pidInput > pidSetpoint);

  // Relay output
  motorPWM = (int)(above ? (128 + atRelay) : (128 - atRelay));
  motorPWM = constrain(motorPWM, 0, 255);

  if (pidInput > atMax) atMax = pidInput;
  if (pidInput < atMin) atMin = pidInput;

  // Deteksi crossing setpoint
  if (above != atLastAbove) {
    if (atCrossings > 0) {
      unsigned long half = now - atLastCross;
      atPeriodSum += half * 2;
    }
    atLastCross  = now;
    atLastAbove  = above;
    atCrossings++;
  }

  // Selesai setelah 10 crossing (~5 osilasi penuh)
  if (atCrossings >= 10) {
    finishTune();
    return;
  }

  // Timeout 40 detik
  if (now - atStart > 40000) {
    Serial.println(F("[AUTOTUNE] Timeout. Gunakan parameter default."));
    atActive = false;
  }
}

void finishTune() {
  double amp = (atMax - atMin) / 2.0;
  if (amp < 0.5) amp = 0.5;

  // Ziegler-Nichols: Ku = 4d/(π·A), Tu = avg_period
  Ku = (4.0 * atRelay) / (3.14159265 * amp);
  Tu = (atPeriodSum / max(atCrossings - 1, 1)) / 1000.0;  // detik

  Kp = constrain(0.60 * Ku,  0.1, 25.0);
  Ki = constrain(1.20 * Ku / Tu, 0.01, 15.0);
  Kd = constrain(0.075 * Ku * Tu, 0.001, 8.0);

  atActive = false;
  atDone   = true;
  errSum   = 0.0;
  lastErr  = 0.0;

  Serial.println(F("\n╔══════════════════════════════╗"));
  Serial.println(F("║     AUTO-TUNE SELESAI!       ║"));
  Serial.printf( "║  Ku=%.3f   Tu=%.3fs        ║\n", Ku, Tu);
  Serial.printf( "║  Kp=%-6.3f  Ki=%-6.3f       ║\n", Kp, Ki);
  Serial.printf( "║  Kd=%-6.3f                  ║\n", Kd);
  Serial.println(F("╚══════════════════════════════╝\n"));

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("TUNE SELESAI!   ");
  lcd.setCursor(0, 1); lcd.printf("Kp%.1f Ki%.1f Kd%.2f", Kp, Ki, Kd);
  delay(3000);
  lcd.clear();
}

// ─────────────────────────────────────────────────────────────
//  SET KECEPATAN MOTOR
// ─────────────────────────────────────────────────────────────
void setMotor(int pwm) {
  pwm     = constrain(pwm, 0, 255);
  pumpPct = map(pwm, 0, 255, 0, 100);
  ledcWrite(MOTOR_ENA, pwm);
}

// ─────────────────────────────────────────────────────────────
//  UPDATE LED STATUS
// ─────────────────────────────────────────────────────────────
void updateLEDs() {
  if (pidInput < LVL_SAFE) {
    digitalWrite(LED_GREEN,  HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED,    LOW);
    statusStr = "AMAN    ";
    alarmOn   = false;
  } else if (pidInput < LVL_WARNING) {
    digitalWrite(LED_GREEN,  LOW);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_RED,    LOW);
    statusStr = "WASPADA ";
    alarmOn   = false;
  } else if (pidInput < LVL_DANGER) {
    digitalWrite(LED_GREEN,  LOW);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_RED,    HIGH);
    statusStr = "SIAGA!! ";
    alarmOn   = false;
  } else {
    digitalWrite(LED_GREEN,  LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED,    HIGH);
    statusStr = "BAHAYA!!";
    alarmOn   = true;
  }
}

// ─────────────────────────────────────────────────────────────
//  BUZZER ALARM (Non-blocking, 300ms toggle)
// ─────────────────────────────────────────────────────────────
void handleBuzzer(unsigned long now) {
  if (!alarmOn) {
    digitalWrite(BUZZER_PIN, LOW);
    return;
  }
  if (now - tBuzz >= 300) {
    tBuzz = now;
    digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN));
  }
}

// ─────────────────────────────────────────────────────────────
//  UPDATE LCD 16x2
// ─────────────────────────────────────────────────────────────
void updateLCD() {
  if (atActive) {
    // Saat autotune
    lcd.setCursor(0, 0);
    lcd.print("AUTO-TUNING...  ");
    lcd.setCursor(0, 1);
    lcd.printf("Cross:%-2d Lvl:%.0f%%  ", atCrossings, pidInput);
    return;
  }

  // Baris 1: icon wave + level air + status
  lcd.setCursor(0, 0);
  lcd.write((uint8_t)0);  // wave
  char buf1[17];
  snprintf(buf1, 17, " Lvl:%5.1f%% %c  ", pidInput,
    (pidInput >= LVL_DANGER) ? '!' : ' ');
  lcd.print(buf1);

  // Baris 2: icon pump + persen pompa + status
  lcd.setCursor(0, 1);
  lcd.write((uint8_t)1);  // pump
  char buf2[17];
  snprintf(buf2, 17, " %s %3d%%  ", statusStr.c_str(), pumpPct);
  lcd.print(buf2);
}

// ─────────────────────────────────────────────────────────────
//  SERIAL OUTPUT
// ─────────────────────────────────────────────────────────────
void printSerial(unsigned long sec) {
  if (atActive) {
    Serial.printf("[%4lu s] [TUNING] Level:%5.1f%% Cross:%-2d Amp:%.2f\n",
      sec, pidInput, atCrossings, (atMax - atMin) / 2.0);
    return;
  }
  Serial.printf("[%4lu s] Lvl:%5.1f%% | Err:%+6.2f | Kp:%.2f Ki:%.2f Kd:%.2f | PWM:%3d | Pump:%3d%% | %s\n",
    sec, pidInput, currentErr, Kp, Ki, Kd, motorPWM, pumpPct, statusStr.c_str());
}

// ─────────────────────────────────────────────────────────────
//  SERIAL COMMAND HANDLER
// ─────────────────────────────────────────────────────────────
void handleSerial() {
  if (!Serial.available()) return;
  char c = toupper(Serial.read());
  while (Serial.available()) Serial.read();

  switch (c) {
    case 'T':
      if (!atActive) {
        atActive  = true;  atDone = false;
        atMax     = -999.0; atMin  = 999.0;
        atCrossings = 0; atPeriodSum = 0;
        atLastAbove = (pidInput > pidSetpoint);
        atStart   = millis(); atLastCross = millis();
        errSum    = 0; lastErr = 0;
        Serial.println(F("\n[AUTOTUNE] Dimulai! Putar potensiometer naik-turun di sekitar 25%."));
        Serial.println(F("[AUTOTUNE] Tunggu ~15-30 detik untuk selesai...\n"));
        lcd.clear(); lcd.setCursor(0,0); lcd.print("AUTOTUNE START! ");
        lcd.setCursor(0,1); lcd.print("Putar POT +/-25%");
      }
      break;
    case 'R':
      Kp=2.5; Ki=0.8; Kd=0.3; errSum=0; lastErr=0;
      Serial.println(F("[CMD] PID reset ke default: Kp=2.5 Ki=0.8 Kd=0.3"));
      break;
    case 'S':
      Serial.println(F("\n──────── STATUS SISTEM ────────"));
      Serial.printf( "  Level Air  : %.2f%%\n", pidInput);
      Serial.printf( "  Setpoint   : %.2f%%\n", pidSetpoint);
      Serial.printf( "  Error      : %.2f\n",   currentErr);
      Serial.printf( "  Motor PWM  : %d/255\n", motorPWM);
      Serial.printf( "  Pump Speed : %d%%\n",   pumpPct);
      Serial.printf( "  Status     : %s\n",      statusStr.c_str());
      Serial.printf( "  Kp=%.4f  Ki=%.4f  Kd=%.4f\n", Kp, Ki, Kd);
      Serial.printf( "  AutoTune   : %s\n", atDone?"Selesai (custom)":"Belum (default)");
      Serial.println(F("───────────────────────────────\n"));
      break;
    case 'H': printHelp(); break;
  }
}

void printHelp() {
  Serial.println(F("\n╔══════════════════════════════════════════════╗"));
  Serial.println(F("║    SMART FLOOD CONTROL - PID Water Pump      ║"));
  Serial.println(F("╠══════════════════════════════════════════════╣"));
  Serial.println(F("║  Perintah Serial Monitor (115200 baud):      ║"));
  Serial.println(F("║  T → Mulai AUTO-TUNE PID                     ║"));
  Serial.println(F("║  R → Reset parameter PID ke default          ║"));
  Serial.println(F("║  S → Tampilkan status sistem lengkap         ║"));
  Serial.println(F("║  H → Tampilkan bantuan ini                   ║"));
  Serial.println(F("╠══════════════════════════════════════════════╣"));
  Serial.println(F("║  Putar Potensiometer untuk simulasi air!     ║"));
  Serial.println(F("║  < 30%  → AMAN  (pompa OFF)                  ║"));
  Serial.println(F("║  30-50% → WASPADA (pompa pelan)              ║"));
  Serial.println(F("║  50-70% → SIAGA  (pompa sedang)              ║"));
  Serial.println(F("║  > 70%  → BAHAYA (pompa MAX + ALARM)         ║"));
  Serial.println(F("╚══════════════════════════════════════════════╝\n"));
}
