#include <Arduino.h>

// ==========================================================
// Simulasi Kontrol Motor DC dengan Self-Tuning PID (ESP32)
// Model motor (diskrit): rpm = rpm + (output - rpm) * 0.1
// ==========================================================

// Variabel utama sistem
float setpoint = 120.0f;   // target RPM
float input = 0.0f;        // feedback RPM (hasil simulasi)
float output = 0.0f;       // PWM 0..255

// Parameter PID (default)
float Kp = 1.20f;
float Ki = 0.35f;
float Kd = 0.08f;

// Internal PID state
float integral = 0.0f;
float prevError = 0.0f;

// Timing loop
const float dt = 0.1f;  // 100 ms
const unsigned long loopDelayMs = 100;

// Batas operasi
const float pwmMin = 0.0f;
const float pwmMax = 255.0f;
const float rpmMin = 0.0f;
const float rpmMax = 255.0f;

// Utility clamp
float clampf(float value, float minVal, float maxVal) {
  if (value < minVal) return minVal;
  if (value > maxVal) return maxVal;
  return value;
}

// PID core
void computePID() {
  float error = setpoint - input;

  // Kandidat integral untuk anti-windup sederhana
  float newIntegral = integral + (error * dt);
  float derivative = (error - prevError) / dt;

  float unsat = (Kp * error) + (Ki * newIntegral) + (Kd * derivative);
  float sat = clampf(unsat, pwmMin, pwmMax);

  // Integrasi hanya saat tidak saturasi berat atau error mendorong keluar saturasi
  bool allowIntegrate = true;
  if ((sat >= pwmMax && error > 0) || (sat <= pwmMin && error < 0)) {
    allowIntegrate = false;
  }
  if (allowIntegrate) {
    integral = newIntegral;
  }

  // Hitung ulang output final memakai integral aktual
  output = (Kp * error) + (Ki * integral) + (Kd * derivative);
  output = clampf(output, pwmMin, pwmMax);
  prevError = error;
}

// Simulasi plant motor DC + noise kecil
void simulateMotor() {
  float noise = random(-20, 21) / 10.0f;  // -2.0 .. 2.0 RPM
  input = input + (output - input) * 0.1f + noise;
  input = clampf(input, rpmMin, rpmMax);
}

// Self-tuning sederhana berbasis besar error saat trigger
void autoTunePID() {
  float e = fabs(setpoint - input);

  if (e > 80.0f) {
    Kp = 2.20f;
    Ki = 0.60f;
    Kd = 0.15f;
  } else if (e > 40.0f) {
    Kp = 1.60f;
    Ki = 0.45f;
    Kd = 0.10f;
  } else {
    Kp = 1.10f;
    Ki = 0.28f;
    Kd = 0.06f;
  }

  // Reset state agar transisi tuning lebih bersih
  integral = 0.0f;
  prevError = 0.0f;

  Serial.print("AUTOTUNE -> ");
  Serial.print("Kp: ");
  Serial.print(Kp, 3);
  Serial.print(" Ki: ");
  Serial.print(Ki, 3);
  Serial.print(" Kd: ");
  Serial.println(Kd, 3);
}

void printHeader() {
  Serial.println("Setpoint|RPM|PWM|Kp|Ki|Kd");
}

void setup() {
  Serial.begin(115200);
  randomSeed(micros());
  delay(300);

  printHeader();
  Serial.println("Ketik angka setpoint (RPM), contoh: 150");
  Serial.println("Ketik 't' untuk trigger autotune.");
}

void handleSerialInput() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd.equalsIgnoreCase("t")) {
    autoTunePID();
    return;
  }

  float newSp = cmd.toFloat();
  // toFloat() akan 0 untuk input tidak valid; tetap diizinkan jika user memang ingin 0
  if (newSp < 0.0f || newSp > 255.0f) {
    Serial.println("Setpoint harus 0..255 RPM");
    return;
  }

  setpoint = newSp;
  Serial.print("Setpoint diubah ke: ");
  Serial.println(setpoint, 2);
}

void loop() {
  handleSerialInput();

  computePID();
  simulateMotor();

  // Format cocok untuk monitor dan plotter
  Serial.print(setpoint, 2);
  Serial.print("|");
  Serial.print(input, 2);
  Serial.print("|");
  Serial.print(output, 2);
  Serial.print("|");
  Serial.print(Kp, 3);
  Serial.print("|");
  Serial.print(Ki, 3);
  Serial.print("|");
  Serial.println(Kd, 3);

  delay(loopDelayMs);
}
