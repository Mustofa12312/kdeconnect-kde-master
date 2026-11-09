#!/usr/bin/env python3
"""
KDE Connect Proximity Auto-Lock & Auto-Wake Service
Monitors Google Pixel 8a presence via KDE Connect D-Bus and LAN ping.
When user walks away with phone for > 15 seconds: locks session.
When user returns: wakes display and notifies user.
"""

import os
import sys
import time
import subprocess
import signal

DEVICE_ID = os.environ.get("KDECONNECT_DEVICE_ID", "3a7731f999984b4993982d7e403b7cdc")
PHONE_IP = os.environ.get("KDECONNECT_PHONE_IP", "192.168.1.12")
TIMEOUT_SECONDS = int(os.environ.get("PROXIMITY_TIMEOUT", "15"))
CHECK_INTERVAL = 3  # seconds

STATE_DIR = os.path.expanduser("~/.config/kdeconnect")
PID_FILE = os.path.join(STATE_DIR, "proximity.pid")
LOG_FILE = os.path.join(STATE_DIR, "proximity.log")
DISABLE_FLAG = os.path.join(STATE_DIR, "proximity_disabled")

def log(msg: str):
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    formatted = f"[{timestamp}] [Proximity] {msg}"
    print(formatted, flush=True)
    try:
        os.makedirs(STATE_DIR, exist_ok=True)
        with open(LOG_FILE, "a") as f:
            f.write(formatted + "\n")
    except Exception:
        pass

def is_disabled() -> bool:
    return os.path.exists(DISABLE_FLAG)

def is_dbus_reachable() -> bool:
    try:
        res = subprocess.run(
            [
                "busctl", "--user", "get-property",
                "org.kde.kdeconnect",
                f"/modules/kdeconnect/devices/{DEVICE_ID}",
                "org.kde.kdeconnect.device",
                "isReachable"
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=2
        )
        if res.returncode == 0 and "true" in res.stdout.lower():
            return True
    except Exception:
        pass
    return False

def is_ping_reachable() -> bool:
    try:
        res = subprocess.run(
            ["ping", "-c", "1", "-W", "1", PHONE_IP],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            timeout=2
        )
        return res.returncode == 0
    except Exception:
        return False

def is_phone_present() -> bool:
    if is_dbus_reachable():
        return True
    return is_ping_reachable()

def is_session_locked() -> bool:
    try:
        res = subprocess.run(
            ["loginctl", "show-session", "", "-p", "LockedHint"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=2
        )
        if "LockedHint=yes" in res.stdout:
            return True
    except Exception:
        pass
    return False

def lock_screen():
    log("Pixel 8a out of range (>15s). Locking laptop screen for security.")
    try:
        subprocess.run(["loginctl", "lock-session"], check=False)
    except Exception as e:
        log(f"Error locking session: {e}")

def wake_screen():
    log("Pixel 8a returned to desk! Laptop auto-wake initiated.")
    try:
        subprocess.run(["busctl", "--user", "call", "org.gnome.Mutter.DisplayConfig", "/org/gnome/Mutter/DisplayConfig", "org.gnome.Mutter.DisplayConfig", "PowerSaveMode", "i", "0"], check=False, stderr=subprocess.DEVNULL)
    except Exception:
        pass

def run_loop():
    log(f"Proximity Auto-Lock daemon active. Target: {PHONE_IP} ({DEVICE_ID}), Timeout: {TIMEOUT_SECONDS}s")
    absent_since = None
    was_absent = False

    while True:
        try:
            if is_disabled():
                time.sleep(CHECK_INTERVAL)
                continue

            present = is_phone_present()

            if present:
                if was_absent:
                    log("Pixel 8a reconnected. Welcome back!")
                    wake_screen()
                    was_absent = False
                absent_since = None
            else:
                now = time.time()
                if absent_since is None:
                    absent_since = now
                    log(f"Pixel 8a signal lost. Counting down ({TIMEOUT_SECONDS}s)...")
                elif (now - absent_since) >= TIMEOUT_SECONDS:
                    if not was_absent:
                        was_absent = True
                        if not is_session_locked():
                            lock_screen()
                        else:
                            log("Session already locked.")

            time.sleep(CHECK_INTERVAL)
        except KeyboardInterrupt:
            break
        except Exception as e:
            log(f"Loop error: {e}")
            time.sleep(CHECK_INTERVAL)

def get_running_pid():
    if not os.path.exists(PID_FILE):
        return None
    try:
        with open(PID_FILE, "r") as f:
            pid = int(f.read().strip())
        os.kill(pid, 0)
        return pid
    except Exception:
        return None

def start_daemon(foreground: bool = False):
    pid = get_running_pid()
    if pid:
        print(f"[Proximity] Daemon already running (PID: {pid}).")
        return

    if foreground:
        with open(PID_FILE, "w") as f:
            f.write(str(os.getpid()))
        try:
            run_loop()
        finally:
            if os.path.exists(PID_FILE):
                os.remove(PID_FILE)
    else:
        proc = subprocess.Popen(
            [sys.executable, os.path.abspath(__file__), "foreground"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            start_new_session=True
        )
        print(f"[Proximity] Started background daemon with PID {proc.pid}.")

def stop_daemon():
    pid = get_running_pid()
    if not pid:
        print("[Proximity] No daemon running.")
        if os.path.exists(PID_FILE):
            os.remove(PID_FILE)
        return
    try:
        os.kill(pid, signal.SIGTERM)
        print(f"[Proximity] Stopped daemon (PID {pid}).")
    except Exception as e:
        print(f"[Proximity] Could not stop PID {pid}: {e}")
    if os.path.exists(PID_FILE):
        os.remove(PID_FILE)

def toggle_service():
    if is_disabled():
        os.remove(DISABLE_FLAG)
        print("[Proximity] Status: AKTIF (Layar laptop akan auto-lock saat HP Pixel 8a menjauh >15 detik).")
    else:
        with open(DISABLE_FLAG, "w") as f:
            f.write("disabled\n")
        print("[Proximity] Status: NONAKTIF (Auto-lock dinonaktifkan sementara).")

def print_status():
    pid = get_running_pid()
    disabled = is_disabled()
    present = is_phone_present()
    locked = is_session_locked()
    dbus_ok = is_dbus_reachable()
    ping_ok = is_ping_reachable()

    print("=== Proximity Auto-Lock Status ===")
    print(f"Daemon Running: {'YES (PID ' + str(pid) + ')' if pid else 'NO'}")
    print(f"Service Enabled: {'NO (Paused)' if disabled else 'YES (Active)'}")
    print(f"Pixel 8a Present: {'YES (At Desk)' if present else 'NO (Away)'}")
    print(f"  - D-Bus Reachable: {'YES' if dbus_ok else 'NO'}")
    print(f"  - LAN Ping ({PHONE_IP}): {'YES' if ping_ok else 'NO'}")
    print(f"Laptop Session Locked: {'YES' if locked else 'NO'}")
    print(f"Timeout Threshold: {TIMEOUT_SECONDS} seconds")

def main():
    action = sys.argv[1] if len(sys.argv) > 1 else "status"
    if action == "start":
        start_daemon(foreground=False)
    elif action == "foreground":
        start_daemon(foreground=True)
    elif action == "stop":
        stop_daemon()
    elif action == "restart":
        stop_daemon()
        time.sleep(1)
        start_daemon(foreground=False)
    elif action == "toggle":
        toggle_service()
    elif action == "status":
        print_status()
    else:
        print("Penggunaan: kdeconnect-proximity [start|stop|restart|toggle|status]")

if __name__ == "__main__":
    main()
