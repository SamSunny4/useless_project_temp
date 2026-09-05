#!/usr/bin/env python3
"""
TinkerHub CyberBot - Raspberry Pi Companion Controller
======================================================
Bi-directional serial communication with the ESP32 DevKit V1 controller.

Features:
- Telemetry listener (status, yaw, 8-sensor ultrasonic distances, motor speeds)
- Emergency obstacle alerts: OBJECT_IN_FRONT, OBJECT_BOTH_SIDES, ALL_SIDES_TRAPPED
- Mode switching: Auto-Evade vs. Raspberry Pi Manual Override
- Interactive keyboard control loop for tank steering override
"""

import sys
import time
import json
import threading

try:
    import serial
except ImportError:
    print("[ERROR] 'pyserial' is required. Install via: pip install pyserial")
    sys.exit(1)

# Default Serial configuration
# On Raspberry Pi GPIO UART: '/dev/serial0' or '/dev/ttyAMA0'
# On USB-Serial adapters: '/dev/ttyUSB0' or 'COM3' (Windows)
DEFAULT_PORT = "/dev/serial0" if sys.platform.startswith("linux") else "COM3"
BAUD_RATE = 115200

# Terminal ANSI Color codes
C_CYAN = "\033[96m"
C_GREEN = "\033[92m"
C_YELLOW = "\033[93m"
C_RED = "\033[91m"
C_BOLD = "\033[1m"
C_RESET = "\033[0m"

class PiCompanion:
    def __init__(self, port=DEFAULT_PORT, baud=BAUD_RATE):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = False
        self.evade_enabled = True
        self.latest_status = "UNKNOWN"
        self.latest_telemetry = {}

    def connect(self):
        try:
            print(f"{C_CYAN}[CONNECTING]{C_RESET} Opening {self.port} at {self.baud} baud...")
            self.ser = serial.Serial(self.port, self.baud, timeout=0.1)
            time.sleep(1.0) # Wait for serial to settle
            print(f"{C_GREEN}[CONNECTED]{C_RESET} Serial connection established.")
            return True
        except Exception as e:
            print(f"{C_RED}[ERROR]{C_RESET} Failed to connect to {self.port}: {e}")
            print("Tip: If using Raspberry Pi GPIO pins 8/10, enable UART in raspi-config.")
            return False

    def send_cmd(self, command: str):
        if not self.ser or not self.ser.is_open:
            return
        cmd_str = command.strip() + "\n"
        self.ser.write(cmd_str.encode("utf-8"))
        self.ser.flush()

    def rx_worker(self):
        while self.running:
            try:
                if self.ser.in_waiting > 0:
                    line = self.ser.readline().decode("utf-8", errors="ignore").strip()
                    if not line:
                        continue

                    if line.startswith("STATUS:"):
                        status = line.split(":", 1)[1]
                        self.latest_status = status
                        self._handle_status_alert(status)

                    elif line.startswith("{") and line.endswith("}"):
                        try:
                            data = json.loads(line)
                            self.latest_telemetry = data
                        except json.JSONDecodeError:
                            pass

                    elif line.startswith("ACK:"):
                        print(f"{C_CYAN}  [ESP32 ACK]{C_RESET} {line}")
            except Exception as e:
                time.sleep(0.05)
            time.sleep(0.01)

    def _handle_status_alert(self, status: str):
        if status == "ALL_SIDES_TRAPPED":
            print(f"\r{C_RED}{C_BOLD}[ALERT: TRAPPED]{C_RESET} All sides blocked! Bot has nowhere to move!")
        elif status == "OBJECT_BOTH_SIDES":
            print(f"\r{C_YELLOW}[WARNING]{C_RESET} Flanks blocked on both sides! Evading corridor...")
        elif status == "OBJECT_IN_FRONT":
            print(f"\r{C_YELLOW}[INFO]{C_RESET} Object detected in FRONT. Rotating away...")
        elif status == "OBJECT_ON_LEFT":
            print(f"\r{C_YELLOW}[INFO]{C_RESET} Object detected on LEFT. Evading right...")
        elif status == "OBJECT_ON_RIGHT":
            print(f"\r{C_YELLOW}[INFO]{C_RESET} Object detected on RIGHT. Evading left...")
        elif status == "OBJECT_IN_REAR":
            print(f"\r{C_YELLOW}[INFO]{C_RESET} Object detected in REAR. Driving forward away...")

    def print_telemetry_hud(self):
        t = self.latest_telemetry
        if not t:
            return
        d = t.get("d", [0]*4)
        yaw = t.get("yaw", 0.0)
        mode = t.get("mode", "UNKNOWN")
        st = self.latest_status

        # Clear line and print compact telemetry
        sys.stdout.write(
            f"\r{C_CYAN}[YAW: {yaw:6.1f}°]{C_RESET} "
            f"Mode: {C_BOLD}{mode:11s}{C_RESET} | "
            f"Status: {st:17s} | "
            f"F:{d[0]:3.0f}cm R:{d[1]:3.0f}cm B:{d[2]:3.0f}cm L:{d[3]:3.0f}cm "
        )
        sys.stdout.flush()

    def start(self):
        if not self.connect():
            return

        self.running = True
        rx_thread = threading.Thread(target=self.rx_worker, daemon=True)
        rx_thread.start()

        print("\n" + "="*60)
        print(f" {C_BOLD}TinkerHub CyberBot - Raspberry Pi Control Station{C_RESET}")
        print("="*60)
        print(" Keyboard Controls:")
        print("   [E] Enable Auto-Evade Mode")
        print("   [D] Disable Auto-Evade & Take Over Manual Control")
        print("   Manual Driving (Tank Steering):")
        print("     [W] Forward      [S] Backward")
        print("     [A] Spin Left    [D] Spin Right")
        print("     [X] Stop Motors")
        print("   [Q] Quit")
        print("="*60 + "\n")

        try:
            while self.running:
                # Telemetry update
                self.print_telemetry_hud()
                time.sleep(0.15)
        except KeyboardInterrupt:
            pass
        finally:
            self.stop()

    def stop(self):
        print("\n[SHUTDOWN] Stopping bot and closing serial...")
        self.running = False
        if self.ser and self.ser.is_open:
            self.send_cmd("CMD:MOVE:STOP")
            time.sleep(0.1)
            self.ser.close()
        print("Done.")

if __name__ == "__main__":
    port_arg = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    app = PiCompanion(port=port_arg)
    app.start()
