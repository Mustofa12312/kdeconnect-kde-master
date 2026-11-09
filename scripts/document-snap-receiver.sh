#!/usr/bin/env bash
#
# Document Snap Receiver for Pixel 8a & KDE Connect
# Watches incoming files from Pixel 8a and auto-sorts documents/scans into workspace.
#

TARGET_DIR="$HOME/Workspace/Research/kdeconnect-kde-master/documents/scans"
INCOMING_DIR="$HOME/Downloads"

mkdir -p "$TARGET_DIR"

echo "[Document-Snap] Folder target siap di: $TARGET_DIR"

# Check if there are newly arrived photos/PDFs in Downloads from KDE Connect
find "$INCOMING_DIR" -maxdepth 1 -type f \( -iname "*.jpg" -o -iname "*.jpeg" -o -iname "*.png" -o -iname "*.pdf" \) -mmin -60 | while read -r file; do
    fname=$(basename "$file")
    if [[ ! -f "$TARGET_DIR/$fname" ]]; then
        cp "$file" "$TARGET_DIR/$fname"
        echo "[Document-Snap] Menyimpan scan dokumen baru: $fname -> $TARGET_DIR/"
        notify-send --app-name="Document Snap" \
            --icon=scanner \
            "📸 Dokumen Snap Diterima" \
            "Berhasil disimpan ke folder proyek:\n$fname"
    fi
done

