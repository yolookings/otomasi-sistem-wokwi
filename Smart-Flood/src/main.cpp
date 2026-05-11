#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- DEKLARASI PIN ---
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int PWM_PIN = 19;
const int BUZZER_PIN = 4;
const int BTN_TUNE = 15;

const int PWM_CHANNEL = 0;
const int PWM_FREQ = 5000;
const int PWM_RES = 8;

// --- VARIABEL PID ---
double Kp = 5.0, Ki = 0.5, Kd = 1.0;
double Setpoint = 40.0; // Target maksimal air di Kolam Retensi (40%)
double Error = 0, Last_Error = 0;
double Integral = 0, Derivative = 0;
double Output_PWM = 0;
double PV_LevelAir = 0; // % Level air aktual (0-100%)

unsigned long lastTime = 0;
int dt = 100;

// --- STATE MACHINE ---
enum SystemState { STANDBY, ACTIVE_PID, EMERGENCY, MAINTENANCE_TUNE };
SystemState currentState = STANDBY;

// Fungsi Menggambar Antarmuka Layar (UI)
void drawDashboard() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Header Status
  display.setCursor(0, 0);
  display.print("SISTEM POLDER: ");
  if (currentState == STANDBY) display.println("STANDBY");
  else if (currentState == ACTIVE_PID) display.println("PID AKTIF");
  else if (currentState == EMERGENCY) display.println("BAHAYA!");
  else if (currentState == MAINTENANCE_TUNE) display.println("AUTO TUNE");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  // Parameter Data
  display.setCursor(0, 15);
  display.print("Batas Aman: "); display.print(Setpoint, 1); display.println(" %");
  display.setCursor(0, 27);
  display.print("Level Air : "); display.print(PV_LevelAir, 1); display.println(" %");
  display.setCursor(0, 39);
  display.print("Daya Pompa: "); display.print((Output_PWM / 255.0) * 100.0, 1); display.println(" %");

  // Bar Indikator Kecepatan Pompa
  display.drawRect(0, 52, 128, 10, SSD1306_WHITE);
  display.fillRect(0, 52, (Output_PWM / 255.0) * 128, 10, SSD1306_WHITE);
  display.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN_TUNE, INPUT_PULLUP);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Gagal memuat OLED"));
  }
  drawDashboard();
}

void loop() {
  unsigned long now = millis();
  
  // 1. CEK EVENT (Event-Based Control)
  if (digitalRead(BTN_TUNE) == LOW) {
    currentState = MAINTENANCE_TUNE;
    Output_PWM = 0; ledcWrite(PWM_CHANNEL, 0); // Matikan pompa sementara
    noTone(BUZZER_PIN);
    drawDashboard();
    
    tone(BUZZER_PIN, 1500, 200); // Suara beep kalibrasi
    delay(2000); // Simulasi sistem melakukan autotuning relay
    
    Integral = 0; 
    currentState = STANDBY;
  }

  // 2. SIKLUS LOOP PID (Timer-Based: Berjalan tiap 100ms)
  if (now - lastTime >= dt && currentState != MAINTENANCE_TUNE) {
    
    // Baca Sensor Ultrasonik
    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    double distance = pulseIn(ECHO_PIN, HIGH) * 0.034 / 2.0; 
    
    // Konversi Jarak ke Persen (Asumsi kedalaman polder maksimal 100cm)
    if (distance > 100) distance = 100;
    PV_LevelAir = 100.0 - distance; 
    
    // Logika State Machine & PID
    if (PV_LevelAir >= 95.0) {
      // RULE: FAIL-SAFE EMERGENCY BYPASS
      currentState = EMERGENCY;
      Output_PWM = 255; 
      tone(BUZZER_PIN, 1000); // Nyalakan Sirine
    } 
    else if (PV_LevelAir > Setpoint) {
      // RULE: PID CLOSED-LOOP AKTIF
      currentState = ACTIVE_PID;
      noTone(BUZZER_PIN);
      
      Error = PV_LevelAir - Setpoint;
      Integral += (Error * (dt / 1000.0));
      Derivative = (Error - Last_Error) / (dt / 1000.0);
      Output_PWM = (Kp * Error) + (Ki * Integral) + (Kd * Derivative);
      
      if (Output_PWM > 255) {
        Output_PWM = 255;
        Integral -= (Error * (dt / 1000.0)); // Anti-Windup
      }
      if (Output_PWM < 50) Output_PWM = 0; // Deadband proteksi pompa
      
      Last_Error = Error;
    } 
    else {
      // RULE: STANDBY
      currentState = STANDBY;
      noTone(BUZZER_PIN);
      Output_PWM = 0;
      Integral = 0;
    }
    
    // Terapkan ke Aktuator dan Perbarui Layar
    ledcWrite(PWM_CHANNEL, (int)Output_PWM);
    drawDashboard();
    lastTime = now;
  }
}