#include "web_admin.h"
#include "sensors.h"
#include "imu.h"
#include "motors.h"
#include "evasion.h"
#include <ArduinoJson.h>

WebAdminManager webAdmin;

// Pure Remote Controller Web Interface (Zero monitoring clutter, pure control & E-Stop)
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>EVade | Remote Controller</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@400;600;700;900&family=JetBrains+Mono:wght@600&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg-base: #080B11;
      --card-bg: rgba(17, 24, 39, 0.85);
      --border-color: rgba(255, 255, 255, 0.1);
      --cyan-neon: #00F0FF;
      --cyan-glow: rgba(0, 240, 255, 0.25);
      --red-neon: #EF4444;
      --red-glow: rgba(239, 68, 68, 0.4);
      --emerald-neon: #10B981;
      --text-primary: #F8FAFC;
      --text-secondary: #94A3B8;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; -webkit-user-select: none; }
    body {
      background: radial-gradient(circle at 50% 0%, #151D2F 0%, var(--bg-base) 80%);
      color: var(--text-primary);
      font-family: 'Outfit', sans-serif;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 16px;
    }

    .container {
      width: 100%;
      max-width: 440px;
      display: flex;
      flex-direction: column;
      gap: 16px;
    }

    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 6px 4px;
    }

    .brand {
      display: flex;
      align-items: center;
      gap: 10px;
    }

    .brand-icon {
      width: 36px;
      height: 36px;
      border-radius: 8px;
      background: linear-gradient(135deg, #00F0FF, #3B82F6);
      display: flex;
      align-items: center;
      justify-content: center;
      font-weight: 900;
      color: #000;
      font-size: 16px;
      box-shadow: 0 0 15px var(--cyan-glow);
    }

    .brand-text h1 { font-size: 17px; font-weight: 700; letter-spacing: 0.5px; }
    .brand-text p { font-size: 11px; color: var(--cyan-neon); font-family: 'JetBrains Mono', monospace; }

    .badge {
      padding: 6px 12px;
      border-radius: 20px;
      font-size: 12px;
      font-weight: 700;
      background: rgba(16, 185, 129, 0.15);
      border: 1px solid var(--emerald-neon);
      color: var(--emerald-neon);
    }
    .badge.estop-alert {
      background: rgba(239, 68, 68, 0.2);
      border-color: var(--red-neon);
      color: var(--red-neon);
      animation: pulseAlert 1s infinite alternate;
    }

    @keyframes pulseAlert {
      from { box-shadow: 0 0 5px var(--red-neon); }
      to { box-shadow: 0 0 18px var(--red-neon); }
    }

    /* EMERGENCY STOP BUTTON */
    .estop-box {
      width: 100%;
    }

    .btn-estop {
      width: 100%;
      padding: 20px;
      border-radius: 16px;
      background: linear-gradient(180deg, #DC2626 0%, #991B1B 100%);
      border: 2px solid #EF4444;
      color: #FFF;
      font-size: 20px;
      font-weight: 900;
      letter-spacing: 2px;
      text-transform: uppercase;
      cursor: pointer;
      box-shadow: 0 6px 25px var(--red-glow);
      transition: all 0.15s ease;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 12px;
    }
    .btn-estop:active {
      transform: scale(0.97);
      background: #7F1D1D;
      box-shadow: 0 2px 10px var(--red-glow);
    }

    .btn-resume {
      width: 100%;
      padding: 16px;
      border-radius: 16px;
      background: linear-gradient(180deg, #059669 0%, #047857 100%);
      border: 2px solid #10B981;
      color: #FFF;
      font-size: 17px;
      font-weight: 800;
      letter-spacing: 1.5px;
      text-transform: uppercase;
      cursor: pointer;
      box-shadow: 0 4px 20px rgba(16, 185, 129, 0.35);
      display: none;
      align-items: center;
      justify-content: center;
      gap: 10px;
    }
    .btn-resume:active { transform: scale(0.97); background: #065F46; }

    /* CARD CONTAINER */
    .card {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      border: 1px solid var(--border-color);
      border-radius: 20px;
      padding: 20px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5);
    }

    .card-title {
      font-size: 12px;
      text-transform: uppercase;
      letter-spacing: 1.5px;
      color: var(--text-secondary);
      margin-bottom: 12px;
      font-weight: 700;
    }

    /* MODE SELECTOR */
    .mode-grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 8px;
    }

    .btn-mode {
      padding: 12px 6px;
      border-radius: 12px;
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid var(--border-color);
      color: var(--text-secondary);
      font-size: 12px;
      font-weight: 700;
      cursor: pointer;
      transition: all 0.2s ease;
      text-align: center;
    }

    .btn-mode.active {
      background: linear-gradient(135deg, rgba(0, 240, 255, 0.2), rgba(59, 130, 246, 0.2));
      border-color: var(--cyan-neon);
      color: var(--cyan-neon);
      box-shadow: 0 0 15px var(--cyan-glow);
    }

    /* TANK CONTROLLER */
    .dpad-container {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 12px;
      margin: 16px auto;
      max-width: 320px;
    }

    .ctrl-btn {
      aspect-ratio: 1;
      border-radius: 16px;
      background: rgba(255, 255, 255, 0.06);
      border: 1px solid var(--border-color);
      color: var(--text-primary);
      font-size: 26px;
      font-weight: bold;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      transition: all 0.1s ease;
      box-shadow: 0 4px 14px rgba(0, 0, 0, 0.35);
      touch-action: none;
    }

    .ctrl-btn:active, .ctrl-btn.active {
      transform: scale(0.92);
      background: var(--cyan-neon);
      color: #000;
      box-shadow: 0 0 25px var(--cyan-neon);
    }

    .ctrl-btn.stop-btn {
      background: rgba(239, 68, 68, 0.15);
      border-color: rgba(239, 68, 68, 0.4);
      color: var(--red-neon);
      font-size: 14px;
      font-weight: 900;
    }
    .ctrl-btn.stop-btn:active { background: var(--red-neon); color: #FFF; }

    /* SLIDERS */
    .slider-group {
      margin-top: 14px;
    }

    .slider-label {
      display: flex;
      justify-content: space-between;
      font-size: 13px;
      color: var(--text-secondary);
      margin-bottom: 6px;
      font-weight: 600;
    }
    .slider-label span { color: var(--cyan-neon); font-family: 'JetBrains Mono', monospace; }

    input[type=range] {
      width: 100%;
      height: 6px;
      border-radius: 4px;
      background: rgba(255, 255, 255, 0.1);
      outline: none;
      -webkit-appearance: none;
    }

    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--cyan-neon);
      cursor: pointer;
      box-shadow: 0 0 10px var(--cyan-neon);
    }

    .disabled-overlay {
      pointer-events: none;
      opacity: 0.35;
      filter: grayscale(0.8);
    }

    /* STALL SAFETY ALERT BANNER */
    .stall-alert-banner {
      background: rgba(239, 68, 68, 0.2);
      border: 1px solid var(--red-neon);
      color: #FCA5A5;
      padding: 8px 12px;
      border-radius: 8px;
      font-size: 11px;
      font-weight: 800;
      text-align: center;
      margin-top: 8px;
      display: none;
      animation: pulseAlert 0.7s infinite alternate;
      letter-spacing: 0.5px;
    }

    /* TRAPPED NO-MOVES ALARM BANNER */
    .alarm-alert-banner {
      background: rgba(239, 68, 68, 0.25);
      border: 2px solid var(--red-neon);
      color: #FFF;
      padding: 10px 14px;
      border-radius: 10px;
      font-size: 12px;
      font-weight: 800;
      text-align: center;
      margin-top: 8px;
      display: none;
      animation: pulseAlert 0.5s infinite alternate;
      letter-spacing: 0.5px;
      box-shadow: 0 0 15px var(--red-glow);
    }

    /* HARDWARE ALARM TOGGLE BUTTON */
    .alarm-box {
      margin-top: 8px;
    }

    .btn-alarm {
      width: 100%;
      padding: 11px 16px;
      border-radius: 12px;
      font-size: 13px;
      font-weight: 800;
      letter-spacing: 0.5px;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      transition: all 0.2s ease;
      background: rgba(245, 158, 11, 0.12);
      border: 1px solid rgba(245, 158, 11, 0.4);
      color: #FCD34D;
      box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
    }
    .btn-alarm:hover {
      background: rgba(245, 158, 11, 0.22);
      border-color: #F59E0B;
    }
    .btn-alarm:active {
      transform: scale(0.98);
    }
    .btn-alarm.active {
      background: rgba(239, 68, 68, 0.3);
      border: 2px solid var(--red-neon);
      color: #FFF;
      box-shadow: 0 0 20px var(--red-glow);
      animation: pulseAlert 0.6s infinite alternate;
    }

    /* ULTRASONIC 8-DIRECTIONAL COMPASS GRID */
    .sensor-compass-grid {
      display: grid;
      grid-template-columns: 1fr 1fr 1fr;
      gap: 6px;
      margin: 6px 0 2px 0;
      align-items: center;
    }

    .sensor-node {
      background: rgba(255, 255, 255, 0.03);
      border: 1px solid var(--border-color);
      border-radius: 10px;
      padding: 6px 4px;
      display: flex;
      flex-direction: column;
      align-items: center;
      transition: all 0.2s ease;
      min-width: 0;
      overflow: hidden;
    }

    .sensor-corner {
      opacity: 0.35;
      filter: grayscale(0.8);
      border-style: dashed;
    }

    .sensor-corner.active-diag {
      opacity: 1;
      filter: none;
      border-style: solid;
    }

    .btn-diag-toggle {
      background: rgba(167, 139, 250, 0.15);
      border: 1px solid rgba(167, 139, 250, 0.4);
      color: #C4B5FD;
      border-radius: 6px;
      padding: 3px 8px;
      font-size: 10px;
      font-weight: 700;
      cursor: pointer;
      transition: all 0.2s;
    }
    .btn-diag-toggle.active {
      background: rgba(167, 139, 250, 0.4);
      color: #FFF;
      box-shadow: 0 0 10px rgba(167, 139, 250, 0.5);
    }

    .sensor-node.warning {
      border-color: #F59E0B;
      background: rgba(245, 158, 11, 0.12);
      box-shadow: 0 0 12px rgba(245, 158, 11, 0.3);
    }

    .sensor-node.danger {
      border-color: var(--red-neon);
      background: rgba(239, 68, 68, 0.2);
      box-shadow: 0 0 16px var(--red-glow);
      animation: pulseAlert 0.8s infinite alternate;
    }

    .sensor-dir {
      font-size: 10px;
      font-weight: 700;
      letter-spacing: 1px;
      color: var(--text-secondary);
      text-transform: uppercase;
    }

    .sensor-val {
      font-size: 15px;
      font-weight: 800;
      font-family: 'JetBrains Mono', monospace;
      color: var(--cyan-neon);
      margin: 2px 0;
    }

    .sensor-node.danger .sensor-val { color: var(--red-neon); }
    .sensor-node.warning .sensor-val { color: #F59E0B; }

    .sensor-meter {
      width: 100%;
      height: 4px;
      background: rgba(255, 255, 255, 0.08);
      border-radius: 2px;
      overflow: hidden;
      margin-top: 2px;
    }

    .meter-bar {
      height: 100%;
      width: 100%;
      background: var(--cyan-neon);
      border-radius: 2px;
      transition: width 0.15s ease, background 0.15s ease;
    }

    .robot-center-icon {
      position: relative;
      width: 44px;
      height: 44px;
      border-radius: 50%;
      background: rgba(0, 240, 255, 0.08);
      border: 1px solid rgba(0, 240, 255, 0.3);
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 20px;
      flex-shrink: 0;
    }

    .robot-pulse {
      position: absolute;
      width: 100%;
      height: 100%;
      border-radius: 50%;
      border: 1px solid var(--cyan-neon);
      opacity: 0.4;
      animation: ping 2s cubic-bezier(0, 0, 0.2, 1) infinite;
    }

    .bot-pointer {
      position: absolute;
      top: -8px;
      font-size: 11px;
      color: var(--cyan-neon);
      text-shadow: 0 0 6px var(--cyan-neon);
    }

    .heading-live-badge {
      font-family: 'JetBrains Mono', monospace;
      font-size: 11px;
      font-weight: 700;
      color: var(--cyan-neon);
      background: rgba(0, 240, 255, 0.08);
      border: 1px solid rgba(0, 240, 255, 0.25);
      border-radius: 6px;
      padding: 4px 8px;
      margin-top: 8px;
      text-align: center;
      letter-spacing: 0.5px;
    }

    /* IMU & ODOMETRY CARD STYLES */
    .imu-stats-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 8px;
      margin-bottom: 10px;
    }

    .imu-stat-box {
      background: rgba(15, 23, 42, 0.65);
      border: 1px solid rgba(255, 255, 255, 0.07);
      border-radius: 10px;
      padding: 9px 10px;
      display: flex;
      flex-direction: column;
      gap: 3px;
    }

    .imu-stat-label {
      font-size: 10px;
      font-weight: 700;
      color: var(--text-secondary);
      letter-spacing: 0.5px;
    }

    .imu-stat-val {
      font-size: 13px;
      font-weight: 800;
      font-family: 'JetBrains Mono', monospace;
      color: var(--text-primary);
    }

    .arena-wrap {
      margin-bottom: 10px;
    }

    .arena-legend {
      display: flex;
      justify-content: space-between;
      font-size: 10px;
      color: var(--text-secondary);
      margin-top: 4px;
      padding: 0 4px;
      font-family: 'JetBrains Mono', monospace;
    }

    .imu-btn-row {
      display: grid;
      grid-template-columns: 1fr 1fr 1fr;
      gap: 6px;
    }

    .btn-action {
      background: rgba(30, 41, 59, 0.85);
      border: 1px solid rgba(0, 240, 255, 0.3);
      color: #E2E8F0;
      padding: 8px 4px;
      border-radius: 8px;
      font-size: 11px;
      font-weight: 700;
      cursor: pointer;
      transition: all 0.15s ease;
      text-align: center;
    }
    .btn-action:hover {
      background: rgba(0, 240, 255, 0.15);
      border-color: var(--cyan-neon);
      color: #FFF;
    }
    .btn-action:active {
      transform: scale(0.96);
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <div class="brand">
        <div class="brand-icon">EV</div>
        <div class="brand-text">
          <h1>EVADE</h1>
          <p>REMOTE CONTROLLER</p>
        </div>
      </div>
      <div class="badge estop-alert" id="systemBadge">E-STOPPED</div>
    </header>

    <!-- EMERGENCY STOP -->
    <div class="estop-box">
      <button class="btn-estop" id="estopBtn" onclick="triggerEstop()" style="display: none;">
        <span>STOP</span> EMERGENCY STOP
      </button>
      <button class="btn-resume" id="resumeBtn" onclick="resumeEstop()" style="display: flex;">
        <span>RESET</span> RESET & RESUME MOTORS
      </button>
    </div>
    <div style="text-align: center; font-size: 11px; color: var(--text-secondary); margin-top: 6px;">Tap <b>[SPACE]</b> for quick E-Stop / Resume</div>

    <!-- HARDWARE TASER CONTROL -->
    <div class="alarm-box">
      <button class="btn-alarm" id="alarmToggleBtn" onclick="toggleAlarm()">
        <span id="alarmIcon"></span> <span id="alarmBtnText">FIRE TASER (D4 TEST)</span>
      </button>
    </div>

    <div class="stall-alert-banner" id="stallAlert">[STALL DETECTED] THROTTLE ACTIVE WITH NO ACCELERATION (1S E-STOP)</div>
    <div class="alarm-alert-banner" id="alarmAlert" onclick="toggleAlarm()" style="cursor: pointer;" title="Click to disarm Taser">[ALERT] NO MOVES AVAILABLE — BOT TRAPPED (HIGH-VOLTAGE TASER ACTIVE ON PIN D4) • <u>TAP TO DISARM</u></div>

    <!-- ULTRASONIC SENSOR RADAR CARD -->
    <div class="card" id="radarCard">
      <div class="card-title" style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
        <span>Ultrasonic Radar (6 Directions)</span>
        <div style="display: flex; gap: 8px; align-items: center;">
          <button class="btn-diag-toggle" id="btnDiagToggle" onclick="toggleDiagonalSensors()">REAR DIAG: OFF</button>
          <span id="obstacleAlert" style="color: var(--emerald-neon); font-size: 11px; font-weight: 700; letter-spacing: 0.5px;">PATH CLEAR</span>
        </div>
      </div>

      <div class="sensor-compass-grid">
        <!-- ROW 1: FL (DISABLED), FRONT (0°), FR (DISABLED) -->
        <div class="sensor-node sensor-corner" id="nodeFL" style="opacity: 0.25; border-style: dotted;">
          <span class="sensor-dir">◤ FL</span>
          <span class="sensor-val" style="color: #64748B; font-size: 10px;">DISABLED</span>
          <div class="sensor-meter"><div class="meter-bar" style="width: 0%;"></div></div>
        </div>
        <div class="sensor-node" id="nodeFront">
          <span class="sensor-dir">▲ FRONT 0°</span>
          <span class="sensor-val" id="valFront">---</span>
          <div class="sensor-meter"><div class="meter-bar" id="barFront"></div></div>
        </div>
        <div class="sensor-node sensor-corner" id="nodeFR" style="opacity: 0.25; border-style: dotted;">
          <span class="sensor-dir">FR ◥</span>
          <span class="sensor-val" style="color: #64748B; font-size: 10px;">DISABLED</span>
          <div class="sensor-meter"><div class="meter-bar" style="width: 0%;"></div></div>
        </div>

        <!-- ROW 2: LEFT (270°), CENTER ROBOT, RIGHT (90°) -->
        <div class="sensor-node" id="nodeLeft">
          <span class="sensor-dir">◀ LEFT 270°</span>
          <span class="sensor-val" id="valLeft">---</span>
          <div class="sensor-meter"><div class="meter-bar" id="barLeft"></div></div>
        </div>
        <div class="robot-center-icon" id="botCenterIcon" style="justify-self: center;">
          <div class="robot-pulse"></div>
          <div class="bot-pointer">▲</div>
          <span style="font-size: 10px; font-weight: 800; letter-spacing: 0.5px;">BOT</span>
        </div>
        <div class="sensor-node" id="nodeRight">
          <span class="sensor-dir">RIGHT 90° ▶</span>
          <span class="sensor-val" id="valRight">---</span>
          <div class="sensor-meter"><div class="meter-bar" id="barRight"></div></div>
        </div>

        <!-- ROW 3: RL (225° - GPIO 26), BACK (180°), RR (135° - GPIO 39) -->
        <div class="sensor-node sensor-corner" id="nodeBL">
          <span class="sensor-dir">◣ RL 225°</span>
          <span class="sensor-val" id="valBL">---</span>
          <div class="sensor-meter"><div class="meter-bar" id="barBL"></div></div>
        </div>
        <div class="sensor-node" id="nodeBack">
          <span class="sensor-dir">▼ BACK 180°</span>
          <span class="sensor-val" id="valBack">---</span>
          <div class="sensor-meter"><div class="meter-bar" id="barBack"></div></div>
        </div>
        <div class="sensor-node sensor-corner" id="nodeBR">
          <span class="sensor-dir">RR 135° ◢</span>
          <span class="sensor-val" id="valBR">---</span>
          <div class="sensor-meter"><div class="meter-bar" id="barBR"></div></div>
        </div>
      </div>
      <div class="heading-live-badge" id="headingLiveBadge">YAW: 000.0° (N)</div>
    </div>

    <!-- 6-DOF IMU & POSITION ODOMETRY CARD -->
    <div class="card" id="imuCard">
      <div class="card-title" style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px;">
        <span>6-DOF IMU & Position Odometry</span>
        <span id="imuStatusBadge" style="font-size: 10px; padding: 3px 8px; border-radius: 8px; background: rgba(16,185,129,0.15); color: var(--emerald-neon); border: 1px solid rgba(16,185,129,0.3); font-weight: 700;">MPU6050 ONLINE</span>
      </div>

      <!-- METRICS GRID -->
      <div class="imu-stats-grid">
        <div class="imu-stat-box">
          <span class="imu-stat-label">HEADING (YAW)</span>
          <span class="imu-stat-val" id="valYaw" style="color: var(--cyan-neon);">0.0° (N)</span>
        </div>
        <div class="imu-stat-box">
          <span class="imu-stat-label">PITCH / ROLL</span>
          <span class="imu-stat-val" id="valPitchRoll">0.0° / 0.0°</span>
        </div>
        <div class="imu-stat-box">
          <span class="imu-stat-label">POSITION (X, Y)</span>
          <span class="imu-stat-val" id="valCoords" style="color: #A78BFA;">0.0, 0.0 cm</span>
        </div>
        <div class="imu-stat-box">
          <span class="imu-stat-label">ANGULAR RATE (ωz)</span>
          <span class="imu-stat-val" id="valRateZ">0.0 °/s</span>
        </div>
      </div>

      <!-- 2D ARENA CANVAS -->
      <div class="arena-wrap">
        <canvas id="arenaCanvas" width="400" height="220" style="width: 100%; border-radius: 12px; background: #060911; border: 1px solid rgba(0, 240, 255, 0.2); cursor: crosshair; display: block;"></canvas>
        <div class="arena-legend">
          <span>Click grid to assume (X, Y) pose</span>
          <span id="arenaScaleLbl">Grid: 25cm/div</span>
        </div>
      </div>

      <!-- POSE & CALIBRATION BUTTONS -->
      <div class="imu-btn-row">
        <button class="btn-action" onclick="resetHeadingOnly()">Zero Heading</button>
        <button class="btn-action" onclick="resetOriginOnly()">Reset (0,0)</button>
        <button class="btn-action" onclick="promptCustomPose()">Set Pose</button>
      </div>
    </div>

    <!-- MAIN CONTROLS CARD -->
    <div class="card disabled-overlay" id="controlsCard">
      <div class="card-title">Control Mode</div>
      <div class="mode-grid" style="grid-template-columns: 1fr 1fr;">
        <button class="btn-mode" id="btnModeAuto" onclick="setMode('AUTO_EVADE')">AUTO EVADE</button>
        <button class="btn-mode active" id="btnModeWeb" onclick="setMode('WEB_OVERRIDE')">MANUAL</button>
      </div>

      <!-- TANK DPAD -->
      <div class="dpad-container" id="dpadPanel">
        <button class="ctrl-btn" onpointerdown="sendMove('pivot_left')" onpointerup="sendMove('stop')">◤</button>
        <button class="ctrl-btn" onpointerdown="sendMove('forward')" onpointerup="sendMove('stop')">▲</button>
        <button class="ctrl-btn" onpointerdown="sendMove('pivot_right')" onpointerup="sendMove('stop')">◥</button>
        
        <button class="ctrl-btn" onpointerdown="sendMove('left')" onpointerup="sendMove('stop')">◀</button>
        <button class="ctrl-btn stop-btn" onclick="sendMove('stop')">STOP</button>
        <button class="ctrl-btn" onpointerdown="sendMove('right')" onpointerup="sendMove('stop')">▶</button>

        <button class="ctrl-btn" style="visibility:hidden;"></button>
        <button class="ctrl-btn" onpointerdown="sendMove('backward')" onpointerup="sendMove('stop')">▼</button>
        <button class="ctrl-btn" style="visibility:hidden;"></button>
      </div>

      <!-- SLIDERS -->
      <div class="slider-group">
        <div class="slider-label">
          <span>Tap Throttle & Speed</span>
          <span id="speedDisplay">180 (60ms pulse / 110ms rest)</span>
        </div>
        <input type="range" id="speedRange" min="50" max="255" value="180" oninput="updateSpeed(this.value)">
      </div>

      <div class="slider-group">
        <div class="slider-label">
          <span>Evade Threshold</span>
          <span id="threshDisplay">25 cm</span>
        </div>
        <input type="range" id="threshRange" min="10" max="100" value="25" oninput="updateThresh(this.value)">
      </div>

      <div class="slider-group">
        <div class="slider-label">
          <span>Stall Accel Threshold</span>
          <span id="stallThreshDisplay">0.06 g</span>
        </div>
        <input type="range" id="stallThreshRange" min="0.01" max="0.25" step="0.01" value="0.06" oninput="updateStallThresh(this.value)">
      </div>
    </div>
  </div>

  <script>
    let activeMode = 'WEB_OVERRIDE';
    let isEstop = true;
    let diagonalEnabled = false;

    async function fetchStatus() {
      try {
        const res = await fetch('/api/status');
        if (!res.ok) return;
        const data = await res.json();

        isEstop = data.estop || false;
        activeMode = data.mode || activeMode;

        // E-Stop UI toggle
        const estopBtn = document.getElementById('estopBtn');
        const resumeBtn = document.getElementById('resumeBtn');
        const controlsCard = document.getElementById('controlsCard');
        const sysBadge = document.getElementById('systemBadge');

        if (isEstop) {
          estopBtn.style.display = 'none';
          resumeBtn.style.display = 'flex';
          controlsCard.classList.add('disabled-overlay');
          if (sysBadge) {
            sysBadge.innerText = 'E-STOPPED';
            sysBadge.classList.add('estop-alert');
          }
        } else {
          estopBtn.style.display = 'flex';
          resumeBtn.style.display = 'none';
          controlsCard.classList.remove('disabled-overlay');
          if (sysBadge) {
            sysBadge.innerText = activeMode === 'AUTO_EVADE' ? 'AUTO EVADE' : 'MANUAL';
            sysBadge.classList.remove('estop-alert');
          }
        }

        // Stall E-Stop safety alert
        const stallBanner = document.getElementById('stallAlert');
        if (stallBanner) {
          stallBanner.style.display = data.stall_estop ? 'block' : 'none';
        }

        // Trapped / No-Moves Hardware Alarm banner & Toggle Button
        const alarmBanner = document.getElementById('alarmAlert');
        const alarmBtn = document.getElementById('alarmToggleBtn');
        const alarmIcon = document.getElementById('alarmIcon');
        const alarmBtnText = document.getElementById('alarmBtnText');
        const isAlarm = data.alarm || false;
        const isManual = data.manual_alarm || false;

        if (alarmBanner) {
          alarmBanner.style.display = isAlarm ? 'block' : 'none';
        }
        if (alarmBtn) {
          alarmBtn.classList.toggle('active', isAlarm);
          if (isAlarm) {
            if (alarmIcon) alarmIcon.innerText = '';
            if (alarmBtnText) alarmBtnText.innerText = isManual ? 'DISARM MANUAL TASER (D4 DISCHARGING)' : 'DISARM DEFENSE TASER (TRAPPED)';
          } else {
            if (alarmIcon) alarmIcon.innerText = '';
            if (alarmBtnText) alarmBtnText.innerText = 'FIRE TASER (D4 TEST)';
          }
        }

        // Stall accel threshold display sync
        if (data.stall_threshold !== undefined) {
          const stDisp = document.getElementById('stallThreshDisplay');
          const stRange = document.getElementById('stallThreshRange');
          if (stDisp && document.activeElement !== stRange) {
            stDisp.innerText = Number(data.stall_threshold).toFixed(2) + ' g';
            if (stRange) stRange.value = data.stall_threshold;
          }
        }

        // Mode button styling
        document.getElementById('btnModeAuto').classList.toggle('active', activeMode === 'AUTO_EVADE');
        document.getElementById('btnModeWeb').classList.toggle('active', activeMode === 'WEB_OVERRIDE');

        // Diagonal sensors toggle state
        if (data.diagonal_enabled !== undefined) {
          diagonalEnabled = data.diagonal_enabled;
        }
        const btnDiag = document.getElementById('btnDiagToggle');
        if (btnDiag) {
          btnDiag.innerText = diagonalEnabled ? 'REAR DIAG: ON' : 'REAR DIAG: OFF';
          btnDiag.classList.toggle('active', diagonalEnabled);
        }

        // Update 6-directional ultrasonic sensor readings
        if (data.d && data.d.length >= 4) {
          const thresh = data.threshold || 25;
          const sensorsList = [
            { id: 'Front', dist: data.d[0], isDiag: false },
            { id: 'Right', dist: data.d[1], isDiag: false },
            { id: 'Back',  dist: data.d[2], isDiag: false },
            { id: 'Left',  dist: data.d[3], isDiag: false }
          ];

          if (data.d.length >= 6) {
            sensorsList.push(
              { id: 'BR', dist: data.d[4], isDiag: true }, // RR 135°
              { id: 'BL', dist: data.d[5], isDiag: true }  // RL 225° (GPIO 26)
            );
          }

          let anyDanger = false;
          let anyWarning = false;

          sensorsList.forEach(s => {
            const node = document.getElementById('node' + s.id);
            const val = document.getElementById('val' + s.id);
            const bar = document.getElementById('bar' + s.id);
            if (!node || !val || !bar) return;

            if (s.isDiag) {
              node.classList.toggle('active-diag', diagonalEnabled);
              if (!diagonalEnabled) {
                val.innerText = 'OFF';
                bar.style.width = '0%';
                node.classList.remove('danger', 'warning');
                return;
              }
            }

            const d = s.dist;
            val.innerText = d >= 300 ? '> 300 cm' : d.toFixed(1) + ' cm';
            const pct = Math.min(100, Math.max(5, (d / 150) * 100));
            bar.style.width = pct + '%';

            node.classList.remove('danger', 'warning');
            if (d < thresh) {
              node.classList.add('danger');
              bar.style.background = 'var(--red-neon)';
              anyDanger = true;
            } else if (d < thresh * 1.5) {
              node.classList.add('warning');
              bar.style.background = '#F59E0B';
              anyWarning = true;
            } else {
              bar.style.background = 'var(--cyan-neon)';
            }
          });

          const alertLbl = document.getElementById('obstacleAlert');
          if (alertLbl) {
            if (anyDanger) {
              alertLbl.innerText = 'OBSTACLE DETECTED';
              alertLbl.style.color = 'var(--red-neon)';
            } else if (anyWarning) {
              alertLbl.innerText = 'PROXIMITY CAUTION';
              alertLbl.style.color = '#F59E0B';
            } else {
              alertLbl.innerText = 'PATH CLEAR';
              alertLbl.style.color = 'var(--emerald-neon)';
            }
          }
        }

        // Update IMU & Odometry Position
        const yaw = data.yaw !== undefined ? data.yaw : 0.0;
        const pitch = data.pitch !== undefined ? data.pitch : 0.0;
        const roll = data.roll !== undefined ? data.roll : 0.0;
        const rateZ = data.rate_z !== undefined ? data.rate_z : 0.0;
        const posX = data.x !== undefined ? data.x : 0.0;
        const posY = data.y !== undefined ? data.y : 0.0;
        const imuConn = data.imu_conn !== undefined ? data.imu_conn : false;

        // Rotate bot in center of radar
        const botIcon = document.getElementById('botCenterIcon');
        if (botIcon) botIcon.style.transform = `rotate(${yaw}deg)`;

        function getCompassDir(deg) {
          const norm = ((deg % 360) + 360) % 360;
          const dirs = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];
          return dirs[Math.round(norm / 45) % 8];
        }

        const headingStr = `${yaw >= 0 ? '+' : ''}${yaw.toFixed(1)}° (${getCompassDir(yaw)})`;
        const headingBadge = document.getElementById('headingLiveBadge');
        if (headingBadge) headingBadge.innerText = `YAW: ${headingStr}`;

        // Update IMU Telemetry Card
        document.getElementById('valYaw').innerText = headingStr;
        document.getElementById('valPitchRoll').innerText = `${pitch >= 0 ? '+' : ''}${pitch.toFixed(1)}° / ${roll >= 0 ? '+' : ''}${roll.toFixed(1)}°`;
        document.getElementById('valCoords').innerText = `X: ${posX.toFixed(1)} | Y: ${posY.toFixed(1)} cm`;
        document.getElementById('valRateZ').innerText = `${rateZ.toFixed(1)} °/s`;

        const imuBadge = document.getElementById('imuStatusBadge');
        if (imuBadge) {
          if (imuConn) {
            imuBadge.innerText = 'MPU6050 ONLINE (0x68)';
            imuBadge.style.color = 'var(--emerald-neon)';
            imuBadge.style.borderColor = 'rgba(16,185,129,0.3)';
          } else {
            imuBadge.innerText = 'KINEMATIC TRACKER';
            imuBadge.style.color = '#F59E0B';
            imuBadge.style.borderColor = 'rgba(245,158,11,0.3)';
          }
        }

        // Draw 2D Position Arena
        drawArena(posX, posY, yaw, data.d);
      } catch (err) {
        const sysBadge = document.getElementById('systemBadge');
        if (sysBadge) sysBadge.innerText = 'OFFLINE';
      }
    }

    function triggerEstop() {
      fetch('/api/estop', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ estop: true })
      }).then(fetchStatus);
    }

    function resumeEstop() {
      fetch('/api/estop', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ estop: false })
      }).then(fetchStatus);
    }

    function toggleAlarm() {
      fetch('/api/alarm', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ toggle: true })
      }).then(fetchStatus);
    }

    function sendMove(dir) {
      if (isEstop) return;
      if (activeMode !== 'WEB_OVERRIDE') {
        setMode('WEB_OVERRIDE');
      }
      fetch('/api/control', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: dir })
      });
    }

    function setMode(mode) {
      if (isEstop) return;
      activeMode = mode;
      fetch('/api/mode', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode: mode })
      }).then(fetchStatus);
    }

    function updateThresh(val) {
      document.getElementById('threshDisplay').innerText = val + ' cm';
      fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ threshold: parseFloat(val) })
      });
    }

    function updateStallThresh(val) {
      const v = parseFloat(val);
      document.getElementById('stallThreshDisplay').innerText = v.toFixed(2) + ' g';
      fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ stall_threshold: v })
      });
    }

    function updateSpeed(val) {
      const v = parseInt(val);
      const onMs = Math.round(25 + ((v - 50) / 205) * (160 - 25));
      const offMs = Math.round(250 - ((v - 50) / 205) * (250 - 60));
      document.getElementById('speedDisplay').innerText = `${v} (${onMs}ms pulse / ${offMs}ms rest)`;
      fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ speed: v })
      });
    }

    // --- 2D Arena & IMU Pose Management ---
    function drawArena(botX, botY, yawDeg, distances) {
      const canvas = document.getElementById('arenaCanvas');
      if (!canvas) return;
      const ctx = canvas.getContext('2d');
      const w = canvas.width;
      const h = canvas.height;
      const cx = w / 2;
      const cy = h / 2;
      const scale = 0.8; // 0.8 px per cm

      ctx.clearRect(0, 0, w, h);

      // 1. Coordinate Grid
      ctx.strokeStyle = 'rgba(255, 255, 255, 0.06)';
      ctx.lineWidth = 1;
      const gridSpacing = 25 * scale; // 25cm grid lines
      for (let x = cx % gridSpacing; x < w; x += gridSpacing) {
        ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke();
      }
      for (let y = cy % gridSpacing; y < h; y += gridSpacing) {
        ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
      }

      // 2. Axis Crosshair (Origin)
      ctx.strokeStyle = 'rgba(0, 240, 255, 0.25)';
      ctx.beginPath(); ctx.moveTo(cx, 0); ctx.lineTo(cx, h); ctx.stroke();
      ctx.beginPath(); ctx.moveTo(0, cy); ctx.lineTo(w, cy); ctx.stroke();

      ctx.fillStyle = 'rgba(0, 240, 255, 0.5)';
      ctx.font = '9px JetBrains Mono, monospace';
      ctx.fillText('(0,0)', cx + 4, cy - 4);

      // 3. Robot Position
      const rx = cx + botX * scale;
      const ry = cy - botY * scale;

      // 4. Sonar Rays from Robot
      if (distances && distances.length >= 4) {
        let activeSensors = [
          { deg: 0, dist: distances[0] },   // Front
          { deg: 90, dist: distances[1] },  // Right
          { deg: 180, dist: distances[2] }, // Back
          { deg: 270, dist: distances[3] }  // Left
        ];
        if (diagonalEnabled && distances.length >= 6) {
          activeSensors.push(
            { deg: 135, dist: distances[4] }, // RR
            { deg: 225, dist: distances[5] }  // RL
          );
        }

        activeSensors.forEach(s => {
          const clamped = Math.min(s.dist, 140);
          const rad = (yawDeg + s.deg - 90) * (Math.PI / 180);
          const tx = rx + clamped * scale * Math.cos(rad);
          const ty = ry + clamped * scale * Math.sin(rad);

          ctx.strokeStyle = s.dist < 25 ? 'rgba(239, 68, 68, 0.7)' : 'rgba(0, 240, 255, 0.25)';
          ctx.lineWidth = 1.5;
          ctx.setLineDash([3, 3]);
          ctx.beginPath(); ctx.moveTo(rx, ry); ctx.lineTo(tx, ty); ctx.stroke();
          ctx.setLineDash([]);

          ctx.fillStyle = s.dist < 25 ? '#EF4444' : '#00F0FF';
          ctx.beginPath(); ctx.arc(tx, ty, s.dist < 25 ? 4 : 2.5, 0, Math.PI * 2); ctx.fill();
        });
      }

      // 5. Robot Chassis
      ctx.save();
      ctx.translate(rx, ry);
      ctx.rotate(yawDeg * (Math.PI / 180));

      ctx.fillStyle = '#1E293B';
      ctx.strokeStyle = '#00F0FF';
      ctx.lineWidth = 2;
      ctx.beginPath();
      if (typeof ctx.roundRect === 'function') {
        ctx.roundRect(-12, -16, 24, 32, 4);
      } else {
        ctx.rect(-12, -16, 24, 32);
      }
      ctx.fill();
      ctx.stroke();

      // Treads
      ctx.fillStyle = '#475569';
      ctx.fillRect(-15, -14, 3, 28);
      ctx.fillRect(12, -14, 3, 28);

      // Forward Indicator
      ctx.fillStyle = '#00F0FF';
      ctx.beginPath();
      ctx.moveTo(0, -14);
      ctx.lineTo(5, -6);
      ctx.lineTo(-5, -6);
      ctx.closePath();
      ctx.fill();

      ctx.restore();
    }

    function toggleDiagonalSensors() {
      diagonalEnabled = !diagonalEnabled;
      fetch('/api/sensors/mode', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ diagonal: diagonalEnabled })
      }).then(fetchStatus);
    }

    function resetHeadingOnly() {
      fetch('/api/imu/reset', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ yaw: 0 })
      }).then(fetchStatus);
    }

    function resetOriginOnly() {
      fetch('/api/imu/reset', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ x: 0, y: 0 })
      }).then(fetchStatus);
    }

    function promptCustomPose() {
      const input = prompt('Enter pose as: X, Y, Yaw (e.g. 10, 20, 90):', '0, 0, 0');
      if (!input) return;
      const parts = input.split(',').map(s => parseFloat(s.trim()));
      if (parts.length >= 3 && !isNaN(parts[0]) && !isNaN(parts[1]) && !isNaN(parts[2])) {
        fetch('/api/imu/reset', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ x: parts[0], y: parts[1], yaw: parts[2] })
        }).then(fetchStatus);
      } else if (parts.length >= 1 && !isNaN(parts[0])) {
        fetch('/api/imu/reset', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ yaw: parts[0] })
        }).then(fetchStatus);
      }
    }

    window.addEventListener('DOMContentLoaded', () => {
      const canvas = document.getElementById('arenaCanvas');
      if (canvas) {
        canvas.addEventListener('click', (e) => {
          const rect = canvas.getBoundingClientRect();
          const clickX = e.clientX - rect.left;
          const clickY = e.clientY - rect.top;
          const scaleX = canvas.width / rect.width;
          const scaleY = canvas.height / rect.height;
          const cx = canvas.width / 2;
          const cy = canvas.height / 2;
          const scale = 0.8;

          const newX = ((clickX * scaleX) - cx) / scale;
          const newY = (cy - (clickY * scaleY)) / scale;

          fetch('/api/imu/reset', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ x: Math.round(newX * 10) / 10, y: Math.round(newY * 10) / 10 })
          }).then(fetchStatus);
        });
      }
    });


    // Keyboard support: Space key for instant E-Stop / Resume
    window.addEventListener('keydown', (e) => {
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') return;
      if (e.key === ' ' || e.code === 'Space') {
        e.preventDefault();
        if (isEstop) resumeEstop();
        else triggerEstop();
        return;
      }
      if (e.repeat || isEstop) return;
      if (e.key === 'ArrowUp' || e.key === 'w') sendMove('forward');
      else if (e.key === 'ArrowDown' || e.key === 's') sendMove('backward');
      else if (e.key === 'ArrowLeft' || e.key === 'a') sendMove('left');
      else if (e.key === 'ArrowRight' || e.key === 'd') sendMove('right');
    });

    window.addEventListener('keyup', (e) => {
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') return;
      if (e.key === ' ' || e.code === 'Space') {
        e.preventDefault();
        return;
      }
      if (isEstop) return;
      if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'w', 'a', 's', 'd'].includes(e.key)) {
        sendMove('stop');
      }
    });

    setInterval(fetchStatus, 200);
    fetchStatus();
  </script>
</body>
</html>
)rawliteral";

WebAdminManager::WebAdminManager()
    : server(WEB_SERVER_PORT),
      activeMode(MODE_WEB_OVERRIDE),
      lastWebCmdTime(0),
      wifiWasConnected(false),
      lastLedBlinkTime(0),
      ledState(false),
      lastDisconnectAlertTime(0) {}

void WebAdminManager::init() {
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);

    Serial.println("[WiFi] Starting Access Point mode...");
    WiFi.mode(WIFI_AP);

    // Launch SoftAP directly
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    localIPStr = WiFi.softAPIP().toString();
    Serial.printf("[WiFi] Access Point Started!\n");
    Serial.printf("       SSID:     %s\n", AP_SSID);
    Serial.printf("       Password: %s\n", AP_PASSWORD);
    Serial.printf("       URL:      http://%s\n", localIPStr.c_str());

    // Configure ArduinoOTA for wireless reprogramming over AP
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]() {
        motors.emergencyStop();
        Serial.println("[OTA] Firmware update initiated over AP. Motors cut off.");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Firmware update complete. Rebooting...");
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]\n", error);
    });
    ArduinoOTA.begin();
    Serial.println("[OTA] ArduinoOTA ready on Access Point.");

    setupRoutes();
    server.begin();
    Serial.printf("[WebAdmin] Controls-only portal running on http://%s\n", localIPStr.c_str());
}

void WebAdminManager::setupRoutes() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
    server.on("/api/control", HTTP_POST, [this]() { handleApiControl(); });
    server.on("/api/estop", HTTP_POST, [this]() { handleApiEstop(); });
    server.on("/api/mode", HTTP_POST, [this]() { handleApiMode(); });
    server.on("/api/config", HTTP_POST, [this]() { handleApiConfig(); });
    server.on("/api/imu/reset", HTTP_POST, [this]() { handleApiImuReset(); });
    server.on("/api/sensors/mode", HTTP_POST, [this]() { handleApiSensorsMode(); });
    server.on("/api/alarm", HTTP_POST, [this]() { handleApiAlarm(); });

    server.onNotFound([]() {
        webAdmin.server.send(404, "text/plain", "Not Found");
    });
}

void WebAdminManager::handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

void WebAdminManager::handleApiStatus() {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<512> doc;
#endif

    doc["mode"] = (activeMode == MODE_WEB_OVERRIDE) ? "WEB_OVERRIDE" : "AUTO_EVADE";
    doc["estop"] = motors.isEmergencyStopped();
    doc["stall_estop"] = motors.isStallEstopActive();
    doc["stall_threshold"] = round(motors.getStallAccelThreshold() * 100.0f) / 100.0f;
    doc["alarm"] = evasion.isAlarmActive();
    doc["manual_alarm"] = evasion.isManualAlarm();
    doc["diagonal_enabled"] = sensors.isDiagonalSetEnabled();
    doc["threshold"] = evasion.getThreshold();
    doc["speed"] = motors.getBaseSpeed();
    doc["tap_on"] = motors.getTapOnMs();
    doc["tap_off"] = motors.getTapOffMs();

    // IMU & 6-DOF Telemetry
    doc["yaw"] = round(imu.getYaw() * 10.0f) / 10.0f;
    doc["pitch"] = round(imu.getPitch() * 10.0f) / 10.0f;
    doc["roll"] = round(imu.getRoll() * 10.0f) / 10.0f;
    doc["rate_z"] = round(imu.getAngularVelocityZ() * 10.0f) / 10.0f;
    doc["imu_conn"] = imu.isConnected();

    // Position Odometry (Dead-reckoning)
    doc["x"] = round(imu.getPosX() * 10.0f) / 10.0f;
    doc["y"] = round(imu.getPosY() * 10.0f) / 10.0f;
    doc["dist"] = round(imu.getTotalDistance() * 10.0f) / 10.0f;

    JsonArray d = doc.createNestedArray("d");
    for (int i = 0; i < NUM_ULTRASONIC_SENSORS; i++) {
        d.add(round(sensors.getDistance(i) * 10.0f) / 10.0f);
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebAdminManager::handleApiEstop() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<128> doc;
#endif
    deserializeJson(doc, server.arg("plain"));

    bool trigger = doc["estop"] | false;
    if (trigger) {
        motors.emergencyStop();
    } else {
        motors.resetEmergencyStop();
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebAdminManager::handleApiControl() {
    if (motors.isEmergencyStopped()) {
        server.send(403, "application/json", "{\"error\":\"E-STOP ACTIVE\"}");
        return;
    }

    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<128> doc;
#endif
    deserializeJson(doc, server.arg("plain"));

    String action = doc["action"] | "stop";
    activeMode = MODE_WEB_OVERRIDE;
    lastWebCmdTime = millis();

    if (action == "forward") motors.forward();
    else if (action == "backward") motors.backward();
    else if (action == "left") motors.rotateLeft();
    else if (action == "right") motors.rotateRight();
    else if (action == "pivot_left") motors.pivotLeft();
    else if (action == "pivot_right") motors.pivotRight();
    else motors.stop();

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebAdminManager::handleApiMode() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<128> doc;
#endif
    deserializeJson(doc, server.arg("plain"));

    String m = doc["mode"] | "AUTO_EVADE";
    if (m == "AUTO_EVADE") {
        activeMode = MODE_AUTO_EVADE;
    } else if (m == "WEB_OVERRIDE") {
        activeMode = MODE_WEB_OVERRIDE;
        motors.stop();
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebAdminManager::handleApiConfig() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<128> doc;
#endif
    deserializeJson(doc, server.arg("plain"));

    if (doc.containsKey("threshold")) {
        float th = doc["threshold"].as<float>();
        evasion.setThreshold(th);
    }
    if (doc.containsKey("speed")) {
        uint8_t spd = doc["speed"].as<uint8_t>();
        motors.setBaseSpeed(spd);
    }
    if (doc.containsKey("tap_on") || doc.containsKey("tap_off")) {
        uint16_t onMs = doc["tap_on"] | motors.getTapOnMs();
        uint16_t offMs = doc["tap_off"] | motors.getTapOffMs();
        motors.setTapTiming(onMs, offMs);
    }
    if (doc.containsKey("stall_threshold")) {
        float st = doc["stall_threshold"].as<float>();
        motors.setStallAccelThreshold(st);
        Serial.printf("[Config] Stall Accel Threshold set to: %.2f g\n", st);
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebAdminManager::handleApiImuReset() {
    float yaw = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
    bool hasYaw = false;
    bool hasPos = false;

    if (server.hasArg("plain")) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc;
#else
        StaticJsonDocument<128> doc;
#endif
        deserializeJson(doc, server.arg("plain"));
        if (doc.containsKey("yaw")) {
            yaw = doc["yaw"].as<float>();
            hasYaw = true;
        }
        if (doc.containsKey("x") || doc.containsKey("y")) {
            x = doc["x"] | imu.getPosX();
            y = doc["y"] | imu.getPosY();
            hasPos = true;
        }
    } else {
        hasYaw = true;
        hasPos = true;
    }

    if (hasYaw && hasPos) {
        imu.setPose(x, y, yaw);
    } else if (hasYaw) {
        imu.resetHeading(yaw);
    } else if (hasPos) {
        imu.resetPosition(x, y);
    } else {
        imu.resetHeading(0.0f);
        imu.resetPosition(0.0f, 0.0f);
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebAdminManager::handleApiSensorsMode() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Missing body");
        return;
    }
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<128> doc;
#endif
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    if (doc.containsKey("diagonal")) {
        bool diag = doc["diagonal"].as<bool>();
        sensors.setDiagonalSetEnabled(diag);
        Serial.printf("[Sensors] Diagonal set (45/135/225/315) %s\n", diag ? "ENABLED (8 Sensors Active)" : "DISABLED (4 Cardinal Sensors)");
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebAdminManager::handleApiAlarm() {
    if (server.hasArg("plain")) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc;
#else
        StaticJsonDocument<128> doc;
#endif
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        if (!error) {
            if (doc.containsKey("state")) {
                String state = doc["state"].as<String>();
                if (state == "on") {
                    evasion.setManualAlarm(true);
                } else if (state == "off") {
                    evasion.setManualAlarm(false);
                    evasion.silenceAlarm();
                } else {
                    evasion.toggleManualAlarm();
                }
            } else if (doc.containsKey("enable")) {
                bool en = doc["enable"].as<bool>();
                if (en) {
                    evasion.setManualAlarm(true);
                } else {
                    evasion.setManualAlarm(false);
                    evasion.silenceAlarm();
                }
            } else {
                evasion.toggleManualAlarm();
            }
        } else {
            evasion.toggleManualAlarm();
        }
    } else {
        evasion.toggleManualAlarm();
    }

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument resp;
#else
    StaticJsonDocument<128> resp;
#endif
    resp["status"] = "ok";
    resp["alarm"] = evasion.isAlarmActive();
    resp["manual_alarm"] = evasion.isManualAlarm();
    String out;
    serializeJson(resp, out);
    server.send(200, "application/json", out);
}

void WebAdminManager::update() {
    ArduinoOTA.handle();
    server.handleClient();

    uint32_t now = millis();
    bool currentlyConnected = isConnected();

    if (currentlyConnected) {
        // WiFi connected: Blink ESP32 onboard LED (toggle every 250ms)
        if (now - lastLedBlinkTime >= 250) {
            lastLedBlinkTime = now;
            ledState = !ledState;
            digitalWrite(PIN_STATUS_LED, ledState ? HIGH : LOW);
        }

        if (!wifiWasConnected) {
            wifiWasConnected = true;
            motors.resetEmergencyStop();
            Serial.printf("[WiFi] Client connected! (Active stations: %d). Emergency stop reset.\n", WiFi.softAPgetStationNum());
        }
    } else {
        // WiFi disconnected: Turn OFF status LED
        if (ledState) {
            ledState = false;
            digitalWrite(PIN_STATUS_LED, LOW);
        }

        // Trigger Emergency Stop when WiFi is disconnected
        if (wifiWasConnected) {
            wifiWasConnected = false;
            motors.emergencyStop();
            Serial.println("\n>>> [WIFI SAFETY] WiFi connection lost! Activating EMERGENCY STOP. <<<");
        } else if (!motors.isEmergencyStopped() && (now - lastDisconnectAlertTime >= 3000)) {
            lastDisconnectAlertTime = now;
            motors.emergencyStop();
            Serial.println(">>> [WIFI SAFETY] Waiting for WiFi connection. EMERGENCY STOP active. <<<");
        }
    }

    // Auto-stop if in Web manual override and no control input received for > 1.5s
    if (activeMode == MODE_WEB_OVERRIDE && (now - lastWebCmdTime > 1500)) {
        motors.stop();
    }
}

bool WebAdminManager::isConnected() const {
    bool apConnected = (WiFi.getMode() & WIFI_MODE_AP) && (WiFi.softAPgetStationNum() > 0);
    bool staConnected = (WiFi.getMode() & WIFI_MODE_STA) && (WiFi.status() == WL_CONNECTED);
    return apConnected || staConnected;
}

RobotControlMode WebAdminManager::getActiveMode() const {
    return activeMode;
}

void WebAdminManager::setActiveMode(RobotControlMode mode) {
    activeMode = mode;
}

bool WebAdminManager::isWebOverrideActive() const {
    return activeMode == MODE_WEB_OVERRIDE;
}
