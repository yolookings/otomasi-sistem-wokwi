# Smart Flood Control - ESP32 (Wokwi)
Proyek simulasi sistem pompa pencegahan banjir berbasis **ESP32** dengan kontrol **PID + Auto-Tuning**.
Input level air disimulasikan menggunakan potensiometer, lalu sistem mengatur kecepatan pompa otomatis.

## Fitur Utama
- Kontrol pompa otomatis berdasarkan level air sungai.
- PID controller dengan parameter default (`Kp`, `Ki`, `Kd`).
- Auto-tuning PID via Serial Monitor (metode relay).
- Indikator status visual (LED) + alarm buzzer.
- Tampilan data ringkas di LCD I2C 16x2.

## Kebutuhan
- macOS/Linux (atau environment lain yang mendukung `bash`)
- `arduino-cli`
- `ripgrep` (`rg`) karena dipakai oleh script helper
- Wokwi extension di VS Code (untuk menjalankan simulasi)

Instalasi cepat (macOS + Homebrew):
```bash
brew install arduino-cli ripgrep
```

## Struktur Penting Proyek
- `sketch.ino` → kode utama ESP32
- `diagram.json` → wiring komponen Wokwi
- `run-wokwi.sh` → compile otomatis + update `wokwi.toml`
- `wokwi.toml` → referensi firmware/ELF untuk simulator
- `build/` → output hasil compile (`.bin`, `.elf`)

## Cara Menjalankan
### 1) Compile dan siapkan file simulasi
```bash
chmod +x run-wokwi.sh
./run-wokwi.sh
```

Script akan:
1. memastikan core ESP32 dan library LiquidCrystal I2C tersedia,
2. compile `sketch.ino`,
3. memperbarui `wokwi.toml` agar menunjuk file firmware terbaru di `build/`.

### 2) Jalankan simulasi
Di VS Code, buka proyek ini lalu jalankan **Run Simulation** (Wokwi).

### 3) (Opsional) Ganti target board ESP32
Contoh untuk ESP32-S3:
```bash
FQBN=esp32:esp32:esp32s3 ./run-wokwi.sh
```

## Kontrol Saat Simulasi
Gunakan potensiometer untuk mengubah level air.

Serial Monitor: `115200 baud`
- `T` → mulai auto-tune PID
- `R` → reset PID ke default
- `S` → tampilkan status sistem
- `H` → bantuan perintah

Status level air:
- `< 30%` → AMAN (pompa OFF)
- `30–50%` → WASPADA (pompa pelan)
- `50–70%` → SIAGA (pompa sedang)
- `> 70%` → BAHAYA (pompa maksimal + alarm)

## Troubleshooting Singkat
- `arduino-cli belum terpasang`  
  Pastikan `arduino-cli` sudah terinstal dan ada di `PATH`.

- `hasil compile .bin/.elf tidak ditemukan`  
  Jalankan ulang `./run-wokwi.sh` dan cek error compile di terminal.

- Library LCD tidak ditemukan  
  Jalankan:
  ```bash
  arduino-cli lib install "LiquidCrystal I2C@1.1.2"
  ```

## Catatan
Folder `build/` dan isi `wokwi.toml` bisa berubah setiap kali compile ulang.
