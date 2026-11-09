#!/usr/bin/env bash
#
# Pixel 8a 4K Wireless Webcam Launcher
# Utilizes Pixel 8a flagship camera sensors via low-latency wireless stream to Linux.
#

PHONE_IP="${KDECONNECT_PHONE_IP:-192.168.1.12}"
ADB_PORT="${ADB_PORT:-5555}"
RES="${WEBCAM_RES:-1920x1080}"
FPS="${WEBCAM_FPS:-60}"

echo "[Pixel-Webcam] Menghubungkan ke Pixel 8a ($PHONE_IP:$ADB_PORT)..."

# Try connecting adb over WiFi
adb connect "$PHONE_IP:$ADB_PORT" 2>&1

# Check if adb sees the device
if adb devices | grep -q "$PHONE_IP:$ADB_PORT.*device"; then
    echo "[Pixel-Webcam] Berhasil terhubung secara nirkabel! Membuka Wireless Camera $RES @ ${FPS}fps..."
    exec scrcpy -s "$PHONE_IP:$ADB_PORT" \
        --video-source=camera \
        --camera-size="$RES" \
        --camera-fps="$FPS" \
        --window-title="Pixel 8a 4K Wireless Camera" \
        --stay-awake \
        --power-off-on-close
else
    echo "[Pixel-Webcam] Peringatan: Port ADB nirkabel ($PHONE_IP:$ADB_PORT) belum aktif di HP."
    echo "[Pixel-Webcam] Tips: Aktifkan 'Wireless debugging' di Opsi Pengembang Pixel 8a, atau sambungkan kabel sekali via 'adb tcpip 5555'."
    # Display notification to user
    notify-send --app-name="Pixel 8a Webcam" \
        --icon=camera-web \
        "Pixel 8a Wireless Webcam" \
        "Aktifkan 'Wireless debugging' pada Pixel 8a untuk streaming kamera berlatensi rendah."
fi
