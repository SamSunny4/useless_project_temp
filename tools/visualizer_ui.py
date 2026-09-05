#!/usr/bin/env python3
"""
TinkerHub CyberBot - USB Serial Telemetry & Action Visualizer UI
================================================================
A real-time Python desktop dashboard that parses and visualizes live telemetry
and robot actions directly from the ESP32 USB Serial port (115200 baud).

Features:
- 8-Direction Ultrasonic Radar Canvas with dynamic proximity color-coding
- Gyro Compass Rose displaying real-time Yaw orientation
- Live Action Taken banner (e.g. Reversing, Evading, Rotating, Trapped, E-Stop)
- Motor PWM bar gauges for Left and Right tank tracks
- Timestamped Action Event Logger
- Raw Serial Monitor drawer with auto-scroll
- Simulation Mode for offline testing
"""

import sys
import math
import time
import re
import queue
import threading
import tkinter as tk
from tkinter import ttk, messagebox

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    serial = None

# Regex patterns matching exact ESP32 USB Serial output
RE_TELEMETRY = re.compile(
    r"\[YAW:\s*([+-]?\d+\.?\d*)°\]\s*\[(.*?)\]\s*\[MOT:L=\s*([+-]?\d+)\s+R=\s*([+-]?\d+)\]\s*\|\s*"
    r"S0\(F\):\s*(\d+\.?\d*)\s+S1\(R\):\s*(\d+\.?\d*)\s+S2\(B\):\s*(\d+\.?\d*)\s+S3\(L\):\s*(\d+\.?\d*)"
)

RE_ALERT = re.compile(
    r">>> \[ALERT: STATUS CHANGED\] >>>\s*([A-Z_]+)\s*\(Threshold:\s*([\d\.]+) cm\)"
)

RE_ESTOP = re.compile(r">>> EMERGENCY STOP ACTIVATED")

# Sensor Sector Definitions (4 Orthogonal Sensors @ 90° Spacing)
SENSOR_NAMES = ["S0 (Front)", "S1 (Right)", "S2 (Back)", "S3 (Left)"]
SENSOR_ANGLES = [0, 90, 180, 270]

# Color Palette (Futuristic Dark Cyber Theme)
BG_DARK = "#090D16"
BG_CARD = "#111827"
BG_CARD_LIGHT = "#1F2937"
BORDER_COLOR = "#374151"
CYAN = "#00F0FF"
GREEN = "#10B981"
AMBER = "#F59E0B"
RED = "#EF4444"
TEXT_WHITE = "#F9FAFB"
TEXT_MUTED = "#9CA3AF"

class BotVisualizerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("TinkerHub CyberBot — Serial Telemetry & Action Visualizer")
        self.root.geometry("1180x780")
        self.root.minsize(1050, 720)
        self.root.configure(bg=BG_DARK)

        # Telemetry State
        self.ser = None
        self.serial_thread = None
        self.running = False
        self.msg_queue = queue.Queue()
        self.sim_mode = False
        self.sim_tick = 0

        self.yaw = 0.0
        self.mode = "DISCONNECTED"
        self.status = "UNKNOWN"
        self.left_pwm = 0
        self.right_pwm = 0
        self.threshold = 25.0
        self.distances = [300.0] * 4
        self.action_text = "SYSTEM IDLE / WAITING FOR SERIAL"
        self.action_color = TEXT_MUTED

        self._build_ui()
        self.refresh_com_ports()

        # Start UI periodic updater loop (50 fps)
        self.root.after(20, self._periodic_ui_update)

    def _build_ui(self):
        # 1. TOP TOOLBAR: Connection Controls
        top_bar = tk.Frame(self.root, bg=BG_CARD, padx=14, pady=10, highlightthickness=1, highlightbackground=BORDER_COLOR)
        top_bar.pack(fill=tk.X, side=tk.TOP)

        title_lbl = tk.Label(top_bar, text="🤖 TINKERHUB CYBERBOT", fg=CYAN, bg=BG_CARD, font=("Segoe UI", 13, "bold"))
        title_lbl.pack(side=tk.LEFT, padx=(0, 16))

        tk.Label(top_bar, text="Port:", fg=TEXT_MUTED, bg=BG_CARD, font=("Segoe UI", 10)).pack(side=tk.LEFT, padx=(6, 2))
        self.port_combo = ttk.Combobox(top_bar, width=12, state="readonly")
        self.port_combo.pack(side=tk.LEFT, padx=4)

        refresh_btn = tk.Button(top_bar, text="⟳", command=self.refresh_com_ports, bg=BG_CARD_LIGHT, fg=TEXT_WHITE, relief=tk.FLAT, padx=6)
        refresh_btn.pack(side=tk.LEFT, padx=(0, 10))

        tk.Label(top_bar, text="Baud:", fg=TEXT_MUTED, bg=BG_CARD, font=("Segoe UI", 10)).pack(side=tk.LEFT, padx=(4, 2))
        self.baud_combo = ttk.Combobox(top_bar, width=8, values=["115200", "9600", "57600"], state="readonly")
        self.baud_combo.set("115200")
        self.baud_combo.pack(side=tk.LEFT, padx=4)

        self.conn_btn = tk.Button(top_bar, text="CONNECT", command=self.toggle_connect, bg=GREEN, fg="#000", font=("Segoe UI", 10, "bold"), relief=tk.FLAT, padx=14)
        self.conn_btn.pack(side=tk.LEFT, padx=12)

        self.sim_btn = tk.Button(top_bar, text="SIMULATION: OFF", command=self.toggle_simulation, bg=BG_CARD_LIGHT, fg=TEXT_MUTED, font=("Segoe UI", 9, "bold"), relief=tk.FLAT, padx=10)
        self.sim_btn.pack(side=tk.LEFT, padx=4)

        # Connection Status Dot
        self.status_dot = tk.Label(top_bar, text="● OFFLINE", fg=RED, bg=BG_CARD, font=("Segoe UI", 10, "bold"))
        self.status_dot.pack(side=tk.RIGHT, padx=6)

        # 2. MAIN BODY (Split into Left Visualization and Right Action/Diagnostics)
        main_body = tk.Frame(self.root, bg=BG_DARK, padx=12, pady=12)
        main_body.pack(fill=tk.BOTH, expand=True)

        # LEFT COLUMN: Radar Canvas & Gyro Compass
        left_col = tk.Frame(main_body, bg=BG_CARD, padx=14, pady=14, highlightthickness=1, highlightbackground=BORDER_COLOR)
        left_col.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 8))

        radar_title = tk.Label(left_col, text="4-DIRECTION ULTRASONIC RADAR & ORIENTATION", fg=TEXT_MUTED, bg=BG_CARD, font=("Segoe UI", 10, "bold"))
        radar_title.pack(anchor=tk.W, pady=(0, 6))

        self.radar_canvas = tk.Canvas(left_col, bg="#070A10", highlightthickness=0)
        self.radar_canvas.pack(fill=tk.BOTH, expand=True)

        # Compass readout below canvas
        self.compass_lbl = tk.Label(left_col, text="HEADING (YAW): +0.0°", fg=CYAN, bg=BG_CARD, font=("Consolas", 12, "bold"))
        self.compass_lbl.pack(pady=(6, 0))

        # RIGHT COLUMN: Action Taken, State Machine, Motor PWM, Event Logs
        right_col = tk.Frame(main_body, bg=BG_DARK)
        right_col.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=(8, 0))

        # ACTION TAKEN HERO BANNER
        action_card = tk.Frame(right_col, bg=BG_CARD, padx=14, pady=12, highlightthickness=1, highlightbackground=BORDER_COLOR)
        action_card.pack(fill=tk.X, pady=(0, 10))

        tk.Label(action_card, text="REAL-TIME ROBOT ACTION TAKEN", fg=TEXT_MUTED, bg=BG_CARD, font=("Segoe UI", 9, "bold")).pack(anchor=tk.W)
        self.action_lbl = tk.Label(action_card, text="IDLE / NO SIGNAL", fg=CYAN, bg=BG_CARD, font=("Segoe UI", 13, "bold"), wraplength=480, justify=tk.LEFT)
        self.action_lbl.pack(anchor=tk.W, pady=(4, 2))

        # STATUS BADGES GRID
        badges_frame = tk.Frame(right_col, bg=BG_CARD, padx=12, pady=12, highlightthickness=1, highlightbackground=BORDER_COLOR)
        badges_frame.pack(fill=tk.X, pady=(0, 10))

        tk.Label(badges_frame, text="OPERATING MODE", fg=TEXT_MUTED, bg=BG_CARD, font=("Segoe UI", 9)).grid(row=0, column=0, sticky=tk.W, padx=6)
        tk.Label(badges_frame, text="OBSTACLE STATUS", fg=TEXT_MUTED, bg=BG_CARD, font=("Segoe UI", 9)).grid(row=0, column=1, sticky=tk.W, padx=6)
        tk.Label(badges_frame, text="THRESHOLD", fg=TEXT_MUTED, bg=BG_CARD, font=("Segoe UI", 9)).grid(row=0, column=2, sticky=tk.W, padx=6)

        self.mode_val_lbl = tk.Label(badges_frame, text="AUTO_EVADE", fg=GREEN, bg=BG_CARD, font=("Segoe UI", 12, "bold"))
        self.mode_val_lbl.grid(row=1, column=0, sticky=tk.W, padx=6, pady=(2, 0))

        self.status_val_lbl = tk.Label(badges_frame, text="CLEAR", fg=GREEN, bg=BG_CARD, font=("Segoe UI", 12, "bold"))
        self.status_val_lbl.grid(row=1, column=1, sticky=tk.W, padx=6, pady=(2, 0))

        self.thresh_val_lbl = tk.Label(badges_frame, text="25.0 cm", fg=CYAN, bg=BG_CARD, font=("Segoe UI", 12, "bold"))
        self.thresh_val_lbl.grid(row=1, column=2, sticky=tk.W, padx=6, pady=(2, 0))

        # MOTOR PWM TELEMETRY BARS
        mot_card = tk.Frame(right_col, bg=BG_CARD, padx=14, pady=12, highlightthickness=1, highlightbackground=BORDER_COLOR)
        mot_card.pack(fill=tk.X, pady=(0, 10))

        tk.Label(mot_card, text="TANK MOTOR CONTROLS (PWM -255 to +255)", fg=TEXT_MUTED, bg=BG_CARD, font=("Segoe UI", 9, "bold")).pack(anchor=tk.W, pady=(0, 8))

        mot_grid = tk.Frame(mot_card, bg=BG_CARD)
        mot_grid.pack(fill=tk.X)

        tk.Label(mot_grid, text="LEFT:", fg=TEXT_WHITE, bg=BG_CARD, font=("Segoe UI", 10, "bold"), width=7, anchor=tk.W).grid(row=0, column=0)
        self.left_mot_bar = ttk.Progressbar(mot_grid, orient=tk.HORIZONTAL, length=240, mode="determinate", maximum=255)
        self.left_mot_bar.grid(row=0, column=1, padx=6, pady=4)
        self.left_mot_txt = tk.Label(mot_grid, text="0", fg=CYAN, bg=BG_CARD, font=("Consolas", 10, "bold"), width=6, anchor=tk.W)
        self.left_mot_txt.grid(row=0, column=2)

        tk.Label(mot_grid, text="RIGHT:", fg=TEXT_WHITE, bg=BG_CARD, font=("Segoe UI", 10, "bold"), width=7, anchor=tk.W).grid(row=1, column=0)
        self.right_mot_bar = ttk.Progressbar(mot_grid, orient=tk.HORIZONTAL, length=240, mode="determinate", maximum=255)
        self.right_mot_bar.grid(row=1, column=1, padx=6, pady=4)
        self.right_mot_txt = tk.Label(mot_grid, text="0", fg=CYAN, bg=BG_CARD, font=("Consolas", 10, "bold"), width=6, anchor=tk.W)
        self.right_mot_txt.grid(row=1, column=2)

        # EVENT HISTORY LOG & RAW SERIAL TABS
        notebook = ttk.Notebook(right_col)
        notebook.pack(fill=tk.BOTH, expand=True)

        # Tab 1: Action History Log
        tab_log = tk.Frame(notebook, bg=BG_CARD)
        notebook.add(tab_log, text="Action & Status Event Log")

        self.event_listbox = tk.Listbox(tab_log, bg="#070A10", fg=TEXT_WHITE, font=("Consolas", 9), selectbackground="#1E293B", relief=tk.FLAT)
        log_scroll = ttk.Scrollbar(tab_log, orient=tk.VERTICAL, command=self.event_listbox.yview)
        self.event_listbox.configure(yscrollcommand=log_scroll.set)
        self.event_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(4, 0), pady=4)
        log_scroll.pack(side=tk.RIGHT, fill=tk.Y, pady=4)

        # Tab 2: Raw Serial Stream
        tab_serial = tk.Frame(notebook, bg=BG_CARD)
        notebook.add(tab_serial, text="Raw USB Serial Console")

        self.serial_text = tk.Text(tab_serial, bg="#070A10", fg="#34D399", font=("Consolas", 9), relief=tk.FLAT, state=tk.DISABLED)
        ser_scroll = ttk.Scrollbar(tab_serial, orient=tk.VERTICAL, command=self.serial_text.yview)
        self.serial_text.configure(yscrollcommand=ser_scroll.set)
        self.serial_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(4, 0), pady=4)
        ser_scroll.pack(side=tk.RIGHT, fill=tk.Y, pady=4)

    def refresh_com_ports(self):
        if not serial:
            self.port_combo["values"] = ["No pyserial"]
            return
        ports = serial.tools.list_ports.comports()
        p_list = [p.device for p in ports]
        self.port_combo["values"] = p_list
        if p_list:
            if "COM13" in p_list:
                self.port_combo.set("COM13")
            else:
                self.port_combo.set(p_list[0])
        else:
            self.port_combo.set("")

    def toggle_connect(self):
        if self.running:
            self.disconnect()
        else:
            self.connect()

    def connect(self):
        port = self.port_combo.get().strip()
        baud = int(self.baud_combo.get().strip() or 115200)

        if not port:
            messagebox.showwarning("Serial Warning", "Please select a valid COM port.")
            return

        try:
            self.ser = serial.Serial(port, baud, timeout=0.1)
            self.running = True
            self.conn_btn.config(text="DISCONNECT", bg=RED)
            self.status_dot.config(text=f"● CONNECTED ({port})", fg=GREEN)
            self._log_event(f"Serial connected to {port} @ {baud} baud")

            self.serial_thread = threading.Thread(target=self._serial_worker, daemon=True)
            self.serial_thread.start()
        except Exception as e:
            messagebox.showerror("Connection Error", f"Could not open {port}:\n{e}")

    def disconnect(self):
        self.running = False
        if self.ser:
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None

        self.conn_btn.config(text="CONNECT", bg=GREEN)
        self.status_dot.config(text="● OFFLINE", fg=RED)
        self._log_event("Serial disconnected")

    def toggle_simulation(self):
        self.sim_mode = not self.sim_mode
        if self.sim_mode:
            self.sim_btn.config(text="SIMULATION: ON", fg=CYAN, bg="#1E3A8A")
            self._log_event("Simulation mode enabled")
        else:
            self.sim_btn.config(text="SIMULATION: OFF", fg=TEXT_MUTED, bg=BG_CARD_LIGHT)
            self._log_event("Simulation mode disabled")

    def _serial_worker(self):
        while self.running and self.ser and self.ser.is_open:
            try:
                line = self.ser.readline().decode("utf-8", errors="replace").strip()
                if line:
                    self.msg_queue.put(line)
            except Exception:
                break
            time.sleep(0.005)

    def _log_event(self, text, color=TEXT_WHITE):
        t_str = time.strftime("%H:%M:%S")
        entry = f"[{t_str}] {text}"
        self.event_listbox.insert(tk.END, entry)
        self.event_listbox.see(tk.END)

    def _append_raw_serial(self, line):
        self.serial_text.configure(state=tk.NORMAL)
        self.serial_text.insert(tk.END, line + "\n")
        self.serial_text.see(tk.END)
        self.serial_text.configure(state=tk.DISABLED)

    def _periodic_ui_update(self):
        # 1. Process all queued serial lines
        while not self.msg_queue.empty():
            line = self.msg_queue.get()
            self._append_raw_serial(line)
            self._parse_line(line)

        # 2. Handle simulation generator if active
        if self.sim_mode:
            self._generate_sim_data()

        # 3. Redraw Radar and Canvas Visualizer
        self._render_radar()

        # Loop again
        self.root.after(20, self._periodic_ui_update)

    def _generate_sim_data(self):
        self.sim_tick += 1
        t = self.sim_tick * 0.05

        # Oscillating yaw
        self.yaw = round(math.sin(t * 0.5) * 45.0, 1)

        # Simulated dynamic distances
        for i in range(4):
            angle_offset = i * (math.pi / 2.0)
            dist = 120.0 + 80.0 * math.sin(t + angle_offset)
            self.distances[i] = round(max(8.0, dist), 1)

        # Trigger obstacle in front periodically
        if (self.sim_tick // 40) % 2 == 1:
            self.distances[0] = 18.0
            self.status = "OBJECT_IN_FRONT"
            self.left_pwm = 0
            self.right_pwm = 190
            self.action_text = "⚡ ACTION: PIVOTING AWAY FROM FRONT OBSTACLE"
            self.action_color = AMBER
        else:
            self.status = "CLEAR"
            self.left_pwm = 0
            self.right_pwm = 0
            self.action_text = "✅ ACTION: HOLDING POSITION (ALL PATHWAYS CLEAR)"
            self.action_color = GREEN

        self.mode = "AUTO_EVADE"
        self._update_badges()

    def _parse_line(self, line):
        # 1. Check for Telemetry Match
        m = RE_TELEMETRY.search(line)
        if m:
            self.yaw = float(m.group(1))
            self.mode = m.group(2).strip()
            self.left_pwm = int(m.group(3))
            self.right_pwm = int(m.group(4))
            for i in range(4):
                self.distances[i] = float(m.group(5 + i))

            self._derive_action_taken()
            self._update_badges()
            return

        # 2. Check for Status Alert Match
        m_alert = RE_ALERT.search(line)
        if m_alert:
            new_status = m_alert.group(1)
            self.threshold = float(m_alert.group(2))
            if new_status != self.status:
                self.status = new_status
                self._log_event(f"STATUS CHANGE: {new_status} (Threshold: {self.threshold} cm)")
            self._derive_action_taken()
            self._update_badges()
            return

        # 3. Check for E-Stop Alert Match
        if RE_ESTOP.search(line):
            self.mode = "EMERG_STOP"
            self._log_event(">>> EMERGENCY STOP ENGAGED <<<")
            self._derive_action_taken()
            self._update_badges()

    def _derive_action_taken(self):
        # Interpret kinematics & state into human action
        l, r = self.left_pwm, self.right_pwm

        if self.mode == "EMERG_STOP":
            self.action_text = "🛑 ACTION: EMERGENCY STOPPED (ALL MOTORS CUT OFF)"
            self.action_color = RED
            return

        if self.status == "ALL_SIDES_TRAPPED":
            self.action_text = "⚠️ ACTION: TRAPPED (BOXED IN - MOTORS HALTED FOR SAFETY)"
            self.action_color = RED
            return

        if l > 0 and r > 0:
            if l == r:
                self.action_text = f"▲ ACTION: MOVING FORWARD (Speed: {l})"
            else:
                self.action_text = f"▲ ACTION: FORWARD CURVING (L:{l}, R:{r})"
            self.action_color = CYAN
        elif l < 0 and r < 0:
            self.action_text = f"▼ ACTION: REVERSING / BACKING AWAY (Speed: {abs(l)})"
            self.action_color = AMBER
        elif l < 0 and r > 0:
            self.action_text = f"◀ ACTION: SPINNING LEFT (EVADING OBSTACLE)"
            self.action_color = AMBER
        elif l > 0 and r < 0:
            self.action_text = f"▶ ACTION: SPINNING RIGHT (EVADING OBSTACLE)"
            self.action_color = AMBER
        elif l == 0 and r > 0:
            self.action_text = f"◤ ACTION: PIVOTING LEFT AROUND OBSTACLE"
            self.action_color = AMBER
        elif l > 0 and r == 0:
            self.action_text = f"◥ ACTION: PIVOTING RIGHT AROUND OBSTACLE"
            self.action_color = AMBER
        else:
            if self.status == "CLEAR":
                self.action_text = "✅ ACTION: HOLDING POSITION (ALL PATHWAYS CLEAR)"
                self.action_color = GREEN
            else:
                self.action_text = f"⏳ ACTION: DECIDING EVASION PATH ({self.status})"
                self.action_color = AMBER

    def _update_badges(self):
        self.action_lbl.config(text=self.action_text, fg=self.action_color)
        self.mode_val_lbl.config(text=self.mode)
        self.status_val_lbl.config(text=self.status)
        self.thresh_val_lbl.config(text=f"{self.threshold:.1f} cm")
        self.compass_lbl.config(text=f"HEADING (YAW): {self.yaw:+6.1f}°")

        # Color-code status
        if self.status == "ALL_SIDES_TRAPPED":
            self.status_val_lbl.config(fg=RED)
        elif "OBJECT" in self.status:
            self.status_val_lbl.config(fg=AMBER)
        else:
            self.status_val_lbl.config(fg=GREEN)

        # Update motor progress bars
        self.left_mot_bar["value"] = abs(self.left_pwm)
        self.right_mot_bar["value"] = abs(self.right_pwm)
        self.left_mot_txt.config(text=f"{self.left_pwm:+4d}")
        self.right_mot_txt.config(text=f"{self.right_pwm:+4d}")

    def _render_radar(self):
        c = self.radar_canvas
        c.delete("all")

        w = c.winfo_width()
        h = c.winfo_height()
        if w <= 10 or h <= 10:
            return

        cx = w / 2
        cy = h / 2
        max_r = min(cx, cy) - 35
        if max_r <= 20:
            return

        # Concentric distance range rings (50cm, 100cm, 150cm, 200cm, 300cm)
        rings = [50, 100, 150, 200, 300]
        for dist_cm in rings:
            r = (dist_cm / 300.0) * max_r
            c.create_oval(cx - r, cy - r, cx + r, cy + r, outline="#1F2937", width=1)
            c.create_text(cx + 4, cy - r + 8, text=f"{dist_cm}cm", fill="#4B5563", font=("Consolas", 8), anchor=tk.W)

        # Safety Threshold Field Circle
        thresh_r = min(max_r, (self.threshold / 300.0) * max_r)
        c.create_oval(cx - thresh_r, cy - thresh_r, cx + thresh_r, cy + thresh_r, outline=RED, width=1, dash=(4, 4))

        # 4 Sonar Beams
        # Canvas angles: 0° is Front (-90° in standard Cartesian), rotating clockwise
        for i in range(len(SENSOR_ANGLES)):
            deg = SENSOR_ANGLES[i] - 90  # 0° (Front) points UP (-90°)
            rad = math.radians(deg)
            dist = self.distances[i]

            beam_len = max_r * min(1.0, max(0.06, dist / 300.0))
            x_end = cx + math.cos(rad) * beam_len
            y_end = cy + math.sin(rad) * beam_len

            # Color coding
            if dist < self.threshold:
                beam_color = RED
            elif dist < self.threshold * 1.5:
                beam_color = AMBER
            else:
                beam_color = CYAN

            # Draw Ray
            c.create_line(cx, cy, x_end, y_end, fill=beam_color, width=2)
            c.create_oval(x_end - 5, y_end - 5, x_end + 5, y_end + 5, fill=beam_color, outline="")

            # Distance Label
            label_r = max_r + 18
            lx = cx + math.cos(rad) * label_r
            ly = cy + math.sin(rad) * label_r
            txt = "CLR" if dist >= 300.0 else f"{int(round(dist))}cm"
            c.create_text(lx, ly, text=txt, fill=beam_color, font=("Consolas", 8, "bold"))

        # Center Chassis & Gyro Yaw Heading Indicator
        c.save_yaw = self.yaw
        yaw_rad = math.radians(self.yaw)

        # Rotating robot chassis outline
        corners = [(-16, -24), (16, -24), (16, 24), (-16, 24)]
        rot_corners = []
        for x, y in corners:
            rx = cx + (x * math.cos(yaw_rad) - y * math.sin(yaw_rad))
            ry = cy + (x * math.sin(yaw_rad) + y * math.cos(yaw_rad))
            rot_corners.extend([rx, ry])

        c.create_polygon(rot_corners, fill="#1E293B", outline=CYAN, width=2)

        # Forward Direction Pointer Arrow
        tip_x = cx - 20 * math.sin(yaw_rad)
        tip_y = cy - 20 * math.cos(yaw_rad)
        base_left_x = cx + (-6 * math.cos(yaw_rad) - (-10) * math.sin(yaw_rad))
        base_left_y = cy + (-6 * math.sin(yaw_rad) + (-10) * math.cos(yaw_rad))
        base_right_x = cx + (6 * math.cos(yaw_rad) - (-10) * math.sin(yaw_rad))
        base_right_y = cy + (6 * math.sin(yaw_rad) + (-10) * math.cos(yaw_rad))
        c.create_polygon(tip_x, tip_y, base_left_x, base_left_y, base_right_x, base_right_y, fill=CYAN, outline="")

if __name__ == "__main__":
    root = tk.Tk()
    app = BotVisualizerApp(root)
    root.mainloop()
