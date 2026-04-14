# Kontrol Motor DC dengan Self-Tuning PID Berbasis IoT (Simulasi Wokwi)

Proyek ini adalah simulasi **closed-loop control** motor DC menggunakan **ESP32** di Wokwi.  
Motor dan encoder **tidak menggunakan hardware nyata**, tetapi dimodelkan secara matematis.

## Ringkasan Sistem

- Mikrokontroler: **ESP32**
- Kontrol: **PID (Kp, Ki, Kd)**
- Plant motor simulasi:
  - `rpm = rpm + (output - rpm) * 0.1`
- Input:
  - Setpoint RPM dari **Serial Monitor**
  - Trigger autotune dengan karakter **`t`**
- Output serial real-time:
  - `Setpoint|RPM|PWM|Kp|Ki|Kd`
- Waktu sampling loop:
  - sekitar **100 ms**

## File Proyek

- `sketch.ino` -> kode utama simulasi PID, model motor, dan serial interface
- `diagram.json` -> diagram Wokwi (ESP32 saja)

## Cara Menjalankan di Wokwi

1. Buka [https://wokwi.com/](https://wokwi.com/).
2. Klik **New Project** lalu pilih **ESP32**.
3. Salin isi file `sketch.ino` dari proyek ini ke editor `sketch.ino` di Wokwi.
4. Salin isi file `diagram.json` dari proyek ini ke editor `diagram.json` di Wokwi.
5. Klik tombol **Start Simulation** (ikon Play).
6. Buka **Serial Monitor**.
7. Pastikan baud rate **115200** (sesuai `Serial.begin(115200)`).

Jika berhasil, Anda akan melihat header dan data seperti:

`Setpoint|RPM|PWM|Kp|Ki|Kd`

## Cara Mengoperasikan Simulasi

### 1) Mengubah Setpoint RPM

- Di Serial Monitor, ketik angka RPM (contoh: `150`) lalu tekan Enter.
- Rentang setpoint valid: `0` sampai `255`.

Contoh:

- input `120` -> target berubah ke 120 RPM
- input `200` -> target berubah ke 200 RPM

### 2) Menjalankan Self-Tuning PID

- Di Serial Monitor, ketik `t` lalu Enter.
- Sistem akan menyesuaikan parameter PID secara sederhana berdasarkan besar error saat itu.
- Nilai PID baru akan dicetak, contoh:
  - `AUTOTUNE -> Kp: 1.600 Ki: 0.450 Kd: 0.100`

## Penjelasan Singkat Cara Kerja

1. `computePID()` menghitung aksi kontrol (PWM) dari selisih setpoint dan RPM aktual.
2. `simulateMotor()` memperbarui nilai RPM berdasarkan model dinamis sederhana motor.
3. Ditambahkan noise kecil (`-2..+2 RPM`) agar respon lebih realistis.
4. Data dikirim tiap ~100 ms ke serial untuk monitoring/plotting.
5. Anti-windup sederhana digunakan agar integrator tidak membuat sistem mudah divergen saat saturasi.

## Membaca Output Serial

Format:

`Setpoint|RPM|PWM|Kp|Ki|Kd`

Contoh:

`150.00|121.33|180.22|1.200|0.350|0.080`

Arti kolom:

- `Setpoint` -> target RPM
- `RPM` -> kecepatan motor hasil simulasi (feedback)
- `PWM` -> output kontrol PID (0..255)
- `Kp Ki Kd` -> parameter PID aktif saat ini

## Penggunaan di Serial Plotter

Anda bisa membuka **Serial Plotter** di Wokwi untuk melihat tren data terhadap waktu:

- Perhatikan bagaimana `RPM` mengikuti `Setpoint`.
- Cek apakah terjadi overshoot berlebihan.
- Perhatikan perubahan `PWM` saat setpoint dinaikkan/diturunkan.

## Skenario Uji yang Direkomendasikan

Lakukan urutan berikut:

1. Jalankan simulasi dengan setpoint default.
2. Ubah setpoint ke `180`.
3. Setelah stabil, turunkan ke `80`.
4. Jalankan autotune (`t`).
5. Ulangi perubahan setpoint, misalnya `140` lalu `200`.

Yang diharapkan:

- RPM mengikuti setpoint secara bertahap.
- Sistem tetap stabil (tidak divergen).
- Setelah autotune, respon bisa berubah (lebih cepat/lebih halus tergantung kondisi error saat tuning).

## Troubleshooting

- **Serial Monitor kosong**
  - Pastikan simulasi sudah Start.
  - Pastikan baud rate 115200.

- **Setpoint tidak berubah**
  - Pastikan menekan Enter setelah mengetik angka.
  - Pastikan nilai di rentang 0..255.

- **Autotune tidak jalan**
  - Ketik tepat `t` lalu Enter.

- **Respon terlalu berosilasi**
  - Coba trigger autotune lagi saat kondisi error berbeda.
  - Atau kurangi manual `Kp`/`Ki` pada kode jika ingin lebih konservatif.

## Catatan

- Proyek ini didesain agar **langsung jalan di Wokwi tanpa library tambahan**.
- Tidak ada komponen motor/encoder fisik karena seluruh plant berbasis simulasi matematis.
- Jika ingin pengembangan lanjut, Anda bisa menambahkan:
  - potensiometer virtual sebagai input setpoint,
  - mode autotune yang lebih formal (misalnya pendekatan Ziegler-Nichols berbasis eksperimen osilasi).
