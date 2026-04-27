#!/usr/bin/env bash
set -euo pipefail

# One-command helper:
# 1) Compile sketch.ino for ESP32
# 2) Detect generated .bin/.elf
# 3) Write wokwi.toml with correct paths

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
FQBN="${FQBN:-esp32:esp32:esp32}"
PROJECT_NAME="$(basename "$PROJECT_DIR")"
TMP_SKETCH_DIR="$PROJECT_DIR/wokwi_tmp_sketch"
TMP_SKETCH_NAME="$(basename "$TMP_SKETCH_DIR")"
TMP_MAIN_INO="$TMP_SKETCH_DIR/$TMP_SKETCH_NAME.ino"

cleanup() {
  rm -rf "$TMP_SKETCH_DIR"
}
trap cleanup EXIT

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "Error: arduino-cli belum terpasang."
  echo "Install: brew install arduino-cli"
  exit 1
fi

if [ ! -f "$PROJECT_DIR/sketch.ino" ]; then
  echo "Error: sketch.ino tidak ditemukan di $PROJECT_DIR"
  exit 1
fi

# Ensure Arduino indexes and ESP32 core are ready
echo "==> Checking Arduino core/index..."
arduino-cli core update-index >/dev/null
if ! arduino-cli core list | rg "^esp32:esp32\\s" >/dev/null; then
  echo "==> Installing missing core: esp32:esp32"
  arduino-cli core install esp32:esp32
fi

# Ensure required library is installed
if ! arduino-cli lib list | rg -i "LiquidCrystal[ _]I2C" >/dev/null; then
  echo "==> Installing missing library: LiquidCrystal I2C@1.1.2"
  arduino-cli lib install "LiquidCrystal I2C@1.1.2"
fi

mkdir -p "$BUILD_DIR"
mkdir -p "$TMP_SKETCH_DIR"
cp "$PROJECT_DIR/sketch.ino" "$TMP_MAIN_INO"

echo "==> Compiling sketch for $FQBN"
arduino-cli compile \
  --fqbn "$FQBN" \
  --output-dir "$BUILD_DIR" \
  "$TMP_SKETCH_DIR"

BIN_FILE="$(ls "$BUILD_DIR"/*.bin 2>/dev/null | head -n 1 || true)"
ELF_FILE="$(ls "$BUILD_DIR"/*.elf 2>/dev/null | head -n 1 || true)"

if [ -z "$BIN_FILE" ] || [ -z "$ELF_FILE" ]; then
  echo "Error: hasil compile .bin/.elf tidak ditemukan di folder build/"
  echo "Cek output compile di atas."
  exit 1
fi

BIN_REL="build/$(basename "$BIN_FILE")"
ELF_REL="build/$(basename "$ELF_FILE")"

cat > "$PROJECT_DIR/wokwi.toml" <<EOF
[wokwi]
version = 1
firmware = "$BIN_REL"
elf = "$ELF_REL"
gdbServerPort = 3333
EOF

echo "==> wokwi.toml berhasil diupdate"
echo "    firmware = $BIN_REL"
echo "    elf      = $ELF_REL"

rm -rf "$TMP_SKETCH_DIR"
echo
echo "Next:"
echo "1) Jalankan Wokwi (Run Simulation) di VS Code"
echo "2) Kalau board beda, set env FQBN dulu:"
echo "   FQBN=esp32:esp32:esp32s3 ./run-wokwi.sh"
