# SMART FLOOD PREVENTION PUMP SYSTEM
**(Sistem Pompa Pencegah Banjir Otomatis Berbasis Self-Tuning PID & IoT)**

## 👥 Tim Pengembang (Kelompok 12)
- **Maulana Ahmad Zahiri** (5027231010)
- **Haidar Rafi Aqyla** (5027231029)
- **Nabiel Nizar Anwari** (5027231087)

---

## 📖 Deskripsi Singkat Proyek
Proyek ini mensimulasikan sebuah sistem otomasi pada kolam retensi (polder) untuk mencegah terjadinya banjir. Sistem menggunakan **Mikrokontroler ESP32** untuk membaca ketinggian air secara *real-time* melalui sensor ultrasonik HC-SR04. 

Sistem ini unggul karena tidak hanya menyalakan/mematikan pompa secara biner (ON/OFF), melainkan menggunakan algoritma **Kontrol Proporsional-Integral-Derivatif (PID)**. Algoritma ini akan menghitung *error* antara target batas aman (Setpoint) dan ketinggian air aktual, kemudian mengatur daya hisap pompa air secara dinamis (menggunakan sinyal PWM) sehingga ketinggian air tetap stabil dan transisi pompa menjadi halus.

Sistem juga dilengkapi dengan Layar OLED untuk pemantauan dasbor lokal, *Buzzer* untuk alarm bahaya, dan tombol untuk mode *Maintenance/Auto-Tune*.

## ✨ Fitur Utama
1. **Dashboard Monitoring Real-time (OLED)**: Menampilkan status sistem, Setpoint (batas aman), persentase level air, dan persentase daya pompa.
2. **State-Machine Terintegrasi**: Memiliki 4 state fungsional (`STANDBY`, `ACTIVE_PID`, `EMERGENCY`, `MAINTENANCE_TUNE`).
3. **Kontrol PID Cerdas**: Daya pompa diatur secara proporsional sesuai dengan laju kenaikan air. Terdapat sistem proteksi *Anti-Windup* dan *Deadband*.
4. **Fail-safe Emergency Mode**: Jika level air mencapai batas kritis (95%), pompa otomatis dipaksa menyala 100% dan sirine (buzzer) menyala berbunyi untuk peringatan bahaya.
5. **Event-Based Control**: Adanya interupsi tombol untuk melakukan transisi secara instan ke mode kalibrasi/pemeliharaan sementara.

---

## 🔌 Skema Pin (Pinout)
| Komponen | Pin ESP32 | Deskripsi |
| :--- | :---: | :--- |
| **HC-SR04 (Ultrasonik)** | `5` | TRIG Pin |
| | `18` | ECHO Pin |
| **Pompa DC / Servo (PWM)**| `19` | Output PWM ke Motor Driver/Servo |
| **Buzzer** | `4` | Output alarm audio |
| **Push Button** | `15` | Tombol kalibrasi (Input Pull-up) |
| **OLED SSD1306 (I2C)** | `SDA (21)`| Jalur Data I2C |
| | `SCL (22)`| Jalur Clock I2C |

> *Catatan: Untuk simulasi di Wokwi, pompa direpresentasikan menggunakan Motor DC / Motor Servo yang menerima sinyal PWM dari pin 19.*

---

## 🚀 Cara Menjalankan Simulasi (Wokwi & PlatformIO)

Proyek ini dibangun menggunakan *framework* Arduino pada **PlatformIO** dan disimulasikan menggunakan **Wokwi Simulator**.

### 1. Persiapan Kebutuhan
- Pastikan Anda menggunakan **Visual Studio Code (VSCode)**.
- Instal ekstensi **PlatformIO IDE**.
- Instal ekstensi **Wokwi Simulator**.

### 2. Membuka Proyek
1. Buka folder proyek `Smart-Flood` di VSCode.
2. Tunggu hingga PlatformIO selesai melakukan instalasi *library* secara otomatis (seperti `Adafruit GFX`, `Adafruit SSD1306`, dll) berdasarkan konfigurasi di `platformio.ini`.

### 3. Menjalankan Simulasi
1. Buka file `diagram.json` (atau `diagram.js`) yang ada di *root* folder proyek.
2. Klik ikon/tombol **"Play" (Start Wokwi Simulation)** yang muncul di pojok kanan atas tab editor, atau tekan `F1` dan cari perintah `Wokwi: Start Simulator`.
3. Simulator Wokwi akan membangun ulang kode (build) dan memunculkan jendela simulasi perangkat keras interaktif.

### 4. Skenario Pengujian Simulasi
Setelah simulasi berjalan, Anda dapat melakukan interaksi berikut untuk menguji fitur alat:

- **Test Normal (STANDBY):** Biarkan sensor ultrasonik berada di jarak lebih dari 60cm (Level air < 40%). Status OLED akan menunjukkan `STANDBY` dan daya pompa `0%`.
- **Test PID Control (ACTIVE_PID):** Klik pada komponen Sensor Ultrasonik di simulator, lalu geser *slider* perlahan untuk mengurangi jarak (misal ke 50cm). Status akan berubah menjadi `PID AKTIF`. Daya pompa (PWM) perlahan-lahan akan naik menyesuaikan peningkatan *level* air.
- **Test Bahaya (EMERGENCY):** Geser slider jarak ultrasonik ke titik yang sangat dekat ( < 5cm / Level air > 95%). OLED akan menampilkan `BAHAYA!`, pompa akan langsung menyala penuh `100%`, dan Buzzer akan berbunyi nyaring.
- **Test Maintenance:** Klik *Push Button* kapan saja. Sistem akan membisukan alarm, mematikan pompa (daya `0%`), layar menampilkan status `AUTO TUNE`, dan buzzer akan mengeluarkan suara *beep* singkat 2 kali sebelum sistem mereset kalkulasi *error* PID-nya.

---

**© 2023 - Kelompok 12 Otomasi Sistem** 
