/*
  WiFi Web Server Brushless Motor Control (ESC)

  A simple web server that controls a Brushless Motor via an ESC.
  This sketch creates an access point and launches a web server.

  Wiring Instructions:
  - Connect the ESC's signal wire (usually white or yellow) to pin 9.
  - Connect the ESC's BEC 5V wire (usually red) to the Arduino's 5V pin.
  - Connect the ESC's BEC GND wire (usually black or brown) to the Arduino's GND
  pin.
  - **WARNING**: If you are powering the Arduino via USB while testing, it is
    recommended to disconnect the ESC's BEC 5V power wire to avoid conflicting
  power sources.
*/

#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include "WiFiS3.h"
#include "arduino_secrets.h"
#include <Servo.h>

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password
int keyIndex = 0;

int led = LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);

void printWiFiStatus();

// LED Matrix
ArduinoLEDMatrix matrix;
String scrollText = ""; // Will hold the IP to scroll
unsigned long lastScrollTime = 0;
const unsigned long SCROLL_INTERVAL = 100; // ms between scroll steps

// ESC Setup
Servo esc;
const int escPin = 9;
int currentThrottle = 1000; // 1000us is stopped for unidirectional ESCs

// Rudder Setup (add a servo for the rudder if you have one)
Servo rudder;
const int rudderPin = 10;
int currentRudder = 0; // -45 to +45 degrees

// Battery Voltage Reading
// Wiring: Use a voltage divider on pin A0 to read battery voltage.
//   Battery+ ---[R1 30kOhm]---+---[R2 10kOhm]--- GND
//                              |
//                             A0
const int batteryPin = A0;
const float VOLTAGE_DIVIDER_RATIO =
    4.0; // Adjust to match your actual resistors
float batteryVoltage = 0.0;

// ESC Temperature Reading (NTC Thermistor)
// Wiring: 5V ---[10kOhm pullup]---+--- GND
//                                  |         |
//                                 A1    NTC thermistor
// Place the NTC thermistor on/near the ESC to measure its temperature.
const int tempPin = A1;
const float THERMISTOR_NOMINAL = 10000.0; // 10kOhm NTC at 25C
const float TEMP_NOMINAL = 25.0;          // Nominal temperature
const float B_COEFFICIENT = 3950.0;       // Beta coefficient of your NTC
const float SERIES_RESISTOR = 10000.0;    // 10kOhm series/pullup resistor
float escTemperature = 0.0;

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect
  }
  Serial.println("Access Point Web Server for ESC");

  pinMode(led, OUTPUT);

  // ===== ESC ARMING SEQUENCE =====
  // Most ESCs arm when they receive MINIMUM throttle (1000us) at power-on.
  // The ESC beeps until it sees this low signal, confirming safe startup.
  esc.attach(escPin);
  esc.writeMicroseconds(1000); // Minimum throttle to arm
  Serial.println("Arming ESC - sending minimum throttle (1000us)...");

  // Send min throttle continuously for 3 seconds to complete arming.
  for (int i = 0; i < 30; i++) {
    esc.writeMicroseconds(1000);
    delay(100);
  }
  Serial.println("ESC armed successfully.");

  // Now move to neutral (1500us) - ESC is armed and ready
  esc.writeMicroseconds(1500);
  delay(500);

  // ===== RUDDER SERVO =====
  rudder.attach(rudderPin);
  rudder.write(90); // Center position (90 degrees = straight ahead)
  Serial.println("Rudder servo attached and centered.");

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  WiFi.config(IPAddress(192, 48, 56, 2));

  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    while (true)
      ;
  }

  // wait 10 seconds for connection:
  delay(10000);

  // Keep ESC at neutral and rudder centered during WiFi wait
  esc.writeMicroseconds(1500);
  rudder.write(90);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();

  // Display IP on LED matrix
  matrix.begin();
  IPAddress ip = WiFi.localIP();
  scrollText = "    " + String(ip[0]) + "." + String(ip[1]) + "." +
               String(ip[2]) + "." + String(ip[3]) + "    ";

  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(80);
  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(scrollText);
  matrix.endText(SCROLL_LEFT);
  matrix.endDraw();
}

// Modern Dark Dashboard UI - Single File, Zero External Dependencies
const char index_html[] = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <title>Boat Dashboard</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <style>
    /* ===== CSS RESET & FOUNDATION ===== */
    *, *::before, *::after { margin:0; padding:0; box-sizing:border-box; -webkit-tap-highlight-color:transparent; }
    html, body { 
      width:100%; height:100%; overflow:hidden; 
      touch-action:manipulation;
      -webkit-user-select:none; user-select:none;
    }

    /* ===== DESIGN TOKENS ===== */
    :root {
      --bg: #0a0e17;
      --bg-surface: #111827;
      --bg-card: rgba(17, 24, 39, 0.85);
      --border: rgba(255, 255, 255, 0.08);
      --border-glow: rgba(56, 189, 248, 0.25);
      --text: #f0f4f8;
      --text-dim: rgba(240, 244, 248, 0.55);
      --text-muted: rgba(240, 244, 248, 0.35);
      --accent: #38bdf8;
      --accent-glow: rgba(56, 189, 248, 0.4);
      --forward: #22d3ee;
      --forward-glow: rgba(34, 211, 238, 0.35);
      --reverse: #f97316;
      --reverse-glow: rgba(249, 115, 22, 0.35);
      --danger: #ef4444;
      --danger-glow: rgba(239, 68, 68, 0.4);
      --success: #22c55e;
      --success-glow: rgba(34, 197, 94, 0.5);
      --warning: #eab308;
      --radius: 16px;
      --radius-sm: 10px;
    }

    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', system-ui, sans-serif;
      background: var(--bg);
      color: var(--text);
      display: flex;
      flex-direction: column;
    }

    /* ===== TELEMETRY HEADER ===== */
    .header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 10px 16px;
      background: var(--bg-surface);
      border-bottom: 1px solid var(--border);
      flex-shrink: 0;
      gap: 8px;
      z-index: 100;
    }
    .header-item {
      display: flex;
      align-items: center;
      gap: 6px;
      font-size: 0.78rem;
      font-weight: 600;
      letter-spacing: 0.03em;
      white-space: nowrap;
    }
    .header-item svg { flex-shrink: 0; }
    .conn-dot {
      width: 9px; height: 9px;
      border-radius: 50%;
      background: var(--danger);
      transition: background 0.3s, box-shadow 0.3s;
    }
    .conn-dot.on {
      background: var(--success);
      box-shadow: 0 0 8px var(--success-glow);
    }
    .ping-val { color: var(--accent); }
    .batt-val { color: var(--text); }
    .batt-icon-fill { transition: width 0.5s; }

    /* ===== MAIN LAYOUT ===== */
    .main {
      flex: 1;
      display: flex;
      flex-direction: row;
      overflow: hidden;
      position: relative;
    }

    /* ===== THROTTLE (Vertical Slider) - LEFT ===== */
    .throttle-zone {
      width: 110px;
      flex-shrink: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 12px 6px 10px;
      border-right: 1px solid var(--border);
      background: rgba(17, 24, 39, 0.5);
      position: relative;
    }
    .throttle-label {
      font-size: 0.65rem;
      font-weight: 700;
      letter-spacing: 0.15em;
      text-transform: uppercase;
      color: var(--text-dim);
      margin-bottom: 6px;
    }
    .throttle-value {
      font-size: 1.6rem;
      font-weight: 800;
      font-variant-numeric: tabular-nums;
      letter-spacing: -0.02em;
      margin-bottom: 8px;
      transition: color 0.2s;
      min-height: 2rem;
      text-align: center;
    }
    .throttle-track-wrap {
      flex: 1;
      width: 100%;
      display: flex;
      justify-content: center;
      position: relative;
      touch-action: none;
    }
    .throttle-track {
      width: 8px;
      height: 100%;
      background: rgba(255,255,255,0.06);
      border-radius: 4px;
      position: relative;
      border: 1px solid var(--border);
    }
    .throttle-fill {
      position: absolute;
      left: 0; right: 0;
      border-radius: 4px;
      transition: background 0.15s;
    }
    .throttle-zero-line {
      position: absolute;
      left: -14px; right: -14px;
      height: 2px;
      background: rgba(255,255,255,0.25);
      pointer-events: none;
    }
    .throttle-zero-label {
      position: absolute;
      right: -30px;
      font-size: 0.55rem;
      font-weight: 700;
      color: var(--text-muted);
      transform: translateY(-50%);
      pointer-events: none;
    }
    .throttle-thumb {
      width: 44px;
      height: 44px;
      border-radius: 50%;
      background: radial-gradient(circle at 35% 35%, rgba(255,255,255,0.2), rgba(255,255,255,0.05));
      border: 2px solid rgba(255,255,255,0.3);
      position: absolute;
      left: 50%;
      transform: translate(-50%, -50%);
      cursor: grab;
      touch-action: none;
      backdrop-filter: blur(8px);
      -webkit-backdrop-filter: blur(8px);
      box-shadow: 0 4px 20px rgba(0,0,0,0.4);
      transition: box-shadow 0.2s, border-color 0.2s;
      z-index: 5;
    }
    .throttle-thumb:active, .throttle-thumb.active {
      cursor: grabbing;
      border-color: var(--accent);
      box-shadow: 0 0 25px var(--accent-glow);
    }
    .throttle-ticks {
      position: absolute;
      top: 0; bottom: 0;
      left: 50%;
      transform: translateX(32px);
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      pointer-events: none;
    }
    .throttle-tick {
      font-size: 0.5rem;
      color: var(--text-muted);
      font-weight: 600;
      line-height: 1;
    }

    /* ===== CENTER AREA ===== */
    .center-zone {
      flex: 1;
      display: flex;
      flex-direction: column;
      overflow: hidden;
    }

    /* ===== COMPASS / BOAT VISUAL ===== */
    .visual-area {
      flex: 1;
      display: flex;
      align-items: center;
      justify-content: center;
      position: relative;
      overflow: hidden;
    }
    .compass-ring {
      width: min(240px, 55vw);
      height: min(240px, 55vw);
      border-radius: 50%;
      border: 2px solid var(--border);
      position: relative;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .compass-dir {
      position: absolute;
      font-size: 0.65rem;
      font-weight: 700;
      color: var(--text-muted);
      letter-spacing: 0.1em;
    }
    .compass-dir.n { top: 8px; }
    .compass-dir.s { bottom: 8px; }
    .compass-dir.e { right: 10px; }
    .compass-dir.w { left: 10px; }
    .boat-svg {
      transition: transform 0.15s ease-out;
    }

    /* ===== RUDDER (Horizontal Slider) - BOTTOM ===== */
    .rudder-zone {
      flex-shrink: 0;
      padding: 10px 20px 16px;
      border-top: 1px solid var(--border);
      background: rgba(17, 24, 39, 0.5);
    }
    .rudder-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 8px;
    }
    .rudder-label {
      font-size: 0.65rem;
      font-weight: 700;
      letter-spacing: 0.15em;
      text-transform: uppercase;
      color: var(--text-dim);
    }
    .rudder-value {
      font-size: 1.1rem;
      font-weight: 800;
      font-variant-numeric: tabular-nums;
      color: var(--accent);
    }
    .rudder-track-wrap {
      width: 100%;
      height: 56px;
      position: relative;
      touch-action: none;
    }
    .rudder-track {
      position: absolute;
      top: 50%;
      left: 0; right: 0;
      height: 8px;
      transform: translateY(-50%);
      background: rgba(255,255,255,0.06);
      border-radius: 4px;
      border: 1px solid var(--border);
    }
    .rudder-fill {
      position: absolute;
      top: 0; bottom: 0;
      border-radius: 4px;
      background: var(--accent);
      transition: background 0.15s;
    }
    .rudder-center-line {
      position: absolute;
      top: -10px; bottom: -10px;
      left: 50%;
      width: 2px;
      background: rgba(255,255,255,0.2);
      transform: translateX(-50%);
      pointer-events: none;
    }
    .rudder-thumb {
      width: 48px;
      height: 48px;
      border-radius: 50%;
      background: radial-gradient(circle at 35% 35%, rgba(255,255,255,0.2), rgba(255,255,255,0.05));
      border: 2px solid rgba(255,255,255,0.3);
      position: absolute;
      top: 50%;
      transform: translate(-50%, -50%);
      cursor: grab;
      touch-action: none;
      backdrop-filter: blur(8px);
      -webkit-backdrop-filter: blur(8px);
      box-shadow: 0 4px 20px rgba(0,0,0,0.4);
      transition: box-shadow 0.2s, border-color 0.2s, left 0.06s linear;
      z-index: 5;
    }
    .rudder-thumb:active, .rudder-thumb.active {
      cursor: grabbing;
      border-color: var(--accent);
      box-shadow: 0 0 25px var(--accent-glow);
    }
    .rudder-thumb.snapping {
      transition: left 0.35s cubic-bezier(0.25, 0.46, 0.45, 0.94), box-shadow 0.2s, border-color 0.2s;
    }
    .rudder-port-label, .rudder-stbd-label {
      position: absolute;
      bottom: 0;
      font-size: 0.55rem;
      font-weight: 700;
      color: var(--text-muted);
      letter-spacing: 0.08em;
      text-transform: uppercase;
    }
    .rudder-port-label { left: 4px; }
    .rudder-stbd-label { right: 4px; }

    /* ===== E-STOP BUTTON ===== */
    .estop-btn {
      position: absolute;
      bottom: 16px;
      right: 16px;
      width: 64px; height: 64px;
      border-radius: 50%;
      border: 2px solid rgba(239, 68, 68, 0.5);
      background: radial-gradient(circle at 40% 40%, rgba(239,68,68,0.3), rgba(239,68,68,0.1));
      backdrop-filter: blur(8px);
      -webkit-backdrop-filter: blur(8px);
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      z-index: 50;
      box-shadow: 0 0 20px rgba(239,68,68,0.15);
      transition: all 0.2s;
    }
    .estop-btn:active {
      transform: scale(0.92);
      background: radial-gradient(circle at 40% 40%, rgba(239,68,68,0.6), rgba(239,68,68,0.3));
      box-shadow: 0 0 35px rgba(239,68,68,0.4);
    }
    .estop-btn svg { pointer-events: none; }

    /* ===== LANDSCAPE ADJUSTMENTS ===== */
    @media (orientation: landscape) {
      .throttle-zone { width: 100px; padding: 8px 4px 6px; }
      .throttle-value { font-size: 1.3rem; }
      .compass-ring { width: min(180px, 32vh); height: min(180px, 32vh); }
      .rudder-zone { padding: 6px 20px 10px; }
      .rudder-track-wrap { height: 48px; }
      .estop-btn { width: 56px; height: 56px; bottom: 10px; right: 10px; }
    }

    /* ===== iPAD / LARGER SCREENS ===== */
    @media (min-width: 768px) {
      .header { padding: 14px 24px; }
      .header-item { font-size: 0.88rem; gap: 8px; }
      .throttle-zone { width: 140px; padding: 18px 8px 14px; }
      .throttle-value { font-size: 2.2rem; }
      .throttle-thumb { width: 56px; height: 56px; }
      .compass-ring { width: min(320px, 50vw); height: min(320px, 50vw); }
      .rudder-zone { padding: 14px 32px 22px; }
      .rudder-value { font-size: 1.4rem; }
      .rudder-thumb { width: 58px; height: 58px; }
      .rudder-track-wrap { height: 64px; }
      .estop-btn { width: 80px; height: 80px; bottom: 22px; right: 22px; }
    }
  </style>
</head>
<body>

  <!-- ===== TELEMETRY HEADER ===== -->
  <div class="header">
    <div class="header-item">
      <div class="conn-dot" id="connDot"></div>
      <span id="connText" style="color:var(--text-dim)">OFFLINE</span>
    </div>
    <div class="header-item">
      <!-- Ping Icon (inline SVG) -->
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="color:var(--accent)">
        <polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/>
      </svg>
      <span class="ping-val" id="pingVal">-- ms</span>
    </div>
    <div class="header-item">
      <!-- Battery Icon (inline SVG) -->
      <svg width="28" height="16" viewBox="0 0 32 18" id="battIcon">
        <rect x="0.5" y="1" width="27" height="16" rx="3" ry="3" fill="none" stroke="rgba(255,255,255,0.4)" stroke-width="1.5"/>
        <rect x="27.5" y="5.5" width="3.5" height="7" rx="1" ry="1" fill="rgba(255,255,255,0.4)"/>
        <rect class="batt-icon-fill" id="battFill" x="2.5" y="3" width="23" height="12" rx="1.5" ry="1.5" fill="var(--success)"/>
      </svg>
      <span class="batt-val" id="battVal">--.- V</span>
    </div>
    <div class="header-item">
      <!-- Temp Icon (inline SVG thermometer) -->
      <svg width="14" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="color:#f97316">
        <path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z"/>
      </svg>
      <span id="tempVal" style="color:#f97316">--&deg;C</span>
    </div>
  </div>

  <!-- ===== MAIN LAYOUT ===== -->
  <div class="main">

    <!-- THROTTLE SLIDER (Vertical) -->
    <div class="throttle-zone">
      <div class="throttle-label">Throttle</div>
      <div class="throttle-value" id="throttleVal">0</div>
      <div class="throttle-track-wrap" id="throttleWrap">
        <div class="throttle-track" id="throttleTrack">
          <div class="throttle-fill" id="throttleFill"></div>
          <div class="throttle-zero-line" id="throttleZero"></div>
          <div class="throttle-thumb" id="throttleThumb"></div>
        </div>
        <div class="throttle-ticks" id="throttleTicks"></div>
      </div>
    </div>

    <!-- CENTER: COMPASS + E-STOP -->
    <div class="center-zone">
      <div class="visual-area">
        <div class="compass-ring">
          <span class="compass-dir n">FWD</span>
          <span class="compass-dir s">REV</span>
          <span class="compass-dir e">STBD</span>
          <span class="compass-dir w">PORT</span>
          <!-- Boat SVG -->
          <svg class="boat-svg" id="boatSvg" width="60" height="90" viewBox="0 0 60 90">
            <path d="M30 5 C30 5 12 25 10 55 C9 68 15 85 30 88 C45 85 51 68 50 55 C48 25 30 5 30 5Z" 
                  fill="rgba(56,189,248,0.15)" stroke="var(--accent)" stroke-width="2" stroke-linejoin="round"/>
            <line x1="30" y1="20" x2="30" y2="70" stroke="rgba(255,255,255,0.15)" stroke-width="1" stroke-dasharray="4 3"/>
            <circle cx="30" cy="50" r="4" fill="var(--accent)" opacity="0.6"/>
          </svg>
        </div>

        <!-- E-STOP -->
        <button class="estop-btn" id="estopBtn" aria-label="Emergency Stop">
          <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="var(--danger)" stroke-width="2.5" stroke-linecap="round">
            <rect x="4" y="4" width="16" height="16" rx="2"/>
          </svg>
        </button>
      </div>

      <!-- RUDDER SLIDER (Horizontal) -->
      <div class="rudder-zone">
        <div class="rudder-header">
          <span class="rudder-label">Rudder</span>
          <span class="rudder-value" id="rudderVal">0&deg;</span>
        </div>
        <div class="rudder-track-wrap" id="rudderWrap">
          <div class="rudder-track" id="rudderTrack">
            <div class="rudder-fill" id="rudderFill"></div>
            <div class="rudder-center-line"></div>
          </div>
          <div class="rudder-thumb" id="rudderThumb"></div>
          <span class="rudder-port-label">PORT</span>
          <span class="rudder-stbd-label">STBD</span>
        </div>
      </div>
    </div>
  </div>

  <script>
  (function() {
    'use strict';

    // ===== STATE =====
    let throttle = 0;    // -25 to 100
    let rudder = 0;      // -45 to +45
    let isConnected = false;
    let pendingSend = false;

    // ===== DOM REFS =====
    const $  = id => document.getElementById(id);
    const connDot     = $('connDot');
    const connText    = $('connText');
    const pingVal     = $('pingVal');
    const battVal     = $('battVal');
    const battFill    = $('battFill');
    const throttleVal = $('throttleVal');
    const throttleWrap= $('throttleWrap');
    const throttleTrack=$('throttleTrack');
    const throttleFill= $('throttleFill');
    const throttleZero= $('throttleZero');
    const throttleThumb=$('throttleThumb');
    const rudderVal   = $('rudderVal');
    const rudderWrap  = $('rudderWrap');
    const rudderTrack = $('rudderTrack');
    const rudderFill  = $('rudderFill');
    const rudderThumb = $('rudderThumb');
    const boatSvg     = $('boatSvg');
    const estopBtn    = $('estopBtn');

    // ===== THROTTLE MAPPING =====
    // ESC dead zone: motor doesn't spin until ~1140us (14% of old range)
    // We skip the dead zone so dashboard 1% = motor just starts spinning
    // Reverse: mirror the dead zone below 1000us
    const DEAD_ZONE = 140; // microseconds of dead zone above/below 1000

    function throttleToMicros(t) {
      if (t === 0) return 1000; // STOP
      if (t > 0) {
        // Forward: 1% → 1140us, 100% → 2000us
        return (1000 + DEAD_ZONE) + Math.round((t / 100) * (2000 - 1000 - DEAD_ZONE));
      } else {
        // Reverse: -1% → 860us, -25% → 640us
        return (1000 - DEAD_ZONE) + Math.round((t / 25) * (1000 - DEAD_ZONE - 640));
      }
    }

    // ===== SLIDER: THROTTLE (VERTICAL) =====
    const THROTTLE_MIN = -25;
    const THROTTLE_MAX = 100;

    function getZeroPercent() {
      // Percentage from top where throttle=0 sits
      // Top=100, Bottom=-25. So 0 is (100-0)/(100+25) = 0.8 from top
      return (THROTTLE_MAX - 0) / (THROTTLE_MAX - THROTTLE_MIN);
    }

    function positionThrottleZero() {
      const pct = getZeroPercent();
      throttleZero.style.top = (pct * 100) + '%';
    }

    function valueToThrottleY(val) {
      // val: -25..100.  Top of track = 100, bottom = -25.
      const pct = (THROTTLE_MAX - val) / (THROTTLE_MAX - THROTTLE_MIN);
      return pct; // 0..1
    }

    function throttleYToValue(pct) {
      // pct: 0(top)..1(bottom)
      return THROTTLE_MAX - pct * (THROTTLE_MAX - THROTTLE_MIN);
    }

    function updateThrottleUI() {
      const pct = valueToThrottleY(throttle);
      const zeroPct = getZeroPercent();
      const trackH = throttleTrack.offsetHeight;
      
      // Position thumb
      throttleThumb.style.top = (pct * trackH) + 'px';

      // Fill from zero line to thumb
      if (throttle >= 0) {
        // Fill upward from zero
        const topPx = pct * trackH;
        const zeroPx = zeroPct * trackH;
        throttleFill.style.top = topPx + 'px';
        throttleFill.style.bottom = (trackH - zeroPx) + 'px';
        throttleFill.style.background = 'var(--forward)';
        throttleFill.style.boxShadow = throttle > 0 ? '0 0 12px var(--forward-glow)' : 'none';
      } else {
        // Fill downward from zero (reverse)
        const topPx = zeroPct * trackH;
        const bottomPx = (1 - pct) * trackH;
        throttleFill.style.top = topPx + 'px';
        throttleFill.style.bottom = bottomPx + 'px';
        throttleFill.style.background = 'var(--reverse)';
        throttleFill.style.boxShadow = '0 0 12px var(--reverse-glow)';
      }

      // Value text
      throttleVal.textContent = throttle;
      if (throttle > 0) {
        throttleVal.style.color = 'var(--forward)';
      } else if (throttle < 0) {
        throttleVal.style.color = 'var(--reverse)';
      } else {
        throttleVal.style.color = 'var(--text)';
      }
    }

    // Throttle drag
    let throttleDragging = false;
    function startThrottleDrag(e) {
      e.preventDefault();
      throttleDragging = true;
      throttleThumb.classList.add('active');
      moveThrottle(e);
    }
    function moveThrottle(e) {
      if (!throttleDragging) return;
      const rect = throttleTrack.getBoundingClientRect();
      const y = (e.touches ? e.touches[0].clientY : e.clientY) - rect.top;
      let pct = Math.max(0, Math.min(1, y / rect.height));
      let val = throttleYToValue(pct);
      // Snap to zero if close
      if (Math.abs(val) < 3) val = 0;
      throttle = Math.round(Math.max(THROTTLE_MIN, Math.min(THROTTLE_MAX, val)));
      updateThrottleUI();
      sendControlData();
    }
    function endThrottleDrag() {
      if (!throttleDragging) return;
      throttleDragging = false;
      throttleThumb.classList.remove('active');
    }

    throttleThumb.addEventListener('mousedown', startThrottleDrag);
    throttleThumb.addEventListener('touchstart', startThrottleDrag, {passive:false});
    // Allow dragging on track too
    throttleTrack.addEventListener('mousedown', startThrottleDrag);
    throttleTrack.addEventListener('touchstart', startThrottleDrag, {passive:false});
    document.addEventListener('mousemove', moveThrottle);
    document.addEventListener('touchmove', moveThrottle, {passive:false});
    document.addEventListener('mouseup', endThrottleDrag);
    document.addEventListener('touchend', endThrottleDrag);

    // ===== SLIDER: RUDDER (HORIZONTAL) =====
    const RUDDER_MIN = -45;
    const RUDDER_MAX = 45;
    let rudderDragging = false;
    let rudderSnapping = false;

    function updateRudderUI() {
      const trackW = rudderTrack.offsetWidth;
      const pct = (rudder - RUDDER_MIN) / (RUDDER_MAX - RUDDER_MIN); // 0=left, 1=right
      const centerPct = 0.5;
      
      // Thumb position
      rudderThumb.style.left = (pct * trackW) + 'px';

      // Fill from center to thumb
      if (rudder >= 0) {
        rudderFill.style.left = (centerPct * 100) + '%';
        rudderFill.style.right = ((1 - pct) * 100) + '%';
      } else {
        rudderFill.style.left = (pct * 100) + '%';
        rudderFill.style.right = ((1 - centerPct) * 100) + '%';
      }

      rudderVal.innerHTML = rudder + '&deg;';

      // Rotate boat SVG
      boatSvg.style.transform = 'rotate(' + rudder + 'deg)';
    }

    function startRudderDrag(e) {
      e.preventDefault();
      rudderDragging = true;
      rudderSnapping = false;
      rudderThumb.classList.remove('snapping');
      rudderThumb.classList.add('active');
      moveRudder(e);
    }
    function moveRudder(e) {
      if (!rudderDragging) return;
      const rect = rudderTrack.getBoundingClientRect();
      const x = (e.touches ? e.touches[0].clientX : e.clientX) - rect.left;
      let pct = Math.max(0, Math.min(1, x / rect.width));
      let val = RUDDER_MIN + pct * (RUDDER_MAX - RUDDER_MIN);
      // Snap to zero if close
      if (Math.abs(val) < 2) val = 0;
      rudder = Math.round(Math.max(RUDDER_MIN, Math.min(RUDDER_MAX, val)));
      updateRudderUI();
      sendControlData();
    }
    function endRudderDrag() {
      if (!rudderDragging) return;
      rudderDragging = false;
      rudderThumb.classList.remove('active');
      // Snap back to zero with animation
      snapRudderToZero();
    }
    function snapRudderToZero() {
      rudderThumb.classList.add('snapping');
      rudder = 0;
      updateRudderUI();
      sendControlData();
      setTimeout(() => rudderThumb.classList.remove('snapping'), 400);
    }

    rudderThumb.addEventListener('mousedown', startRudderDrag);
    rudderThumb.addEventListener('touchstart', startRudderDrag, {passive:false});
    rudderTrack.addEventListener('mousedown', startRudderDrag);
    rudderTrack.addEventListener('touchstart', startRudderDrag, {passive:false});
    document.addEventListener('mousemove', function(e) { moveRudder(e); });
    document.addEventListener('touchmove', function(e) { if(rudderDragging) { e.preventDefault(); moveRudder(e); } }, {passive:false});
    document.addEventListener('mouseup', endRudderDrag);
    document.addEventListener('touchend', endRudderDrag);

    // ===== E-STOP =====
    estopBtn.addEventListener('click', function() {
      throttle = 0;
      rudder = 0;
      updateThrottleUI();
      rudderThumb.classList.add('snapping');
      updateRudderUI();
      setTimeout(() => rudderThumb.classList.remove('snapping'), 400);
      sendControlData();
    });

    // ===== COMMUNICATION =====
    function sendControlData() {
      if (pendingSend) return;
      pendingSend = true;
      const micros = throttleToMicros(throttle);
      const t0 = performance.now();
      fetch('/api/throttle?v=' + micros + '&r=' + rudder)
        .then(function(r) {
          if (!r.ok) throw new Error('net');
          return r.json();
        })
        .then(function(data) {
          const ping = Math.round(performance.now() - t0);
          pingVal.textContent = ping + ' ms';
          setConnected(true);
          updateTelemetry(data);
          pendingSend = false;
        })
        .catch(function() {
          setConnected(false);
          pendingSend = false;
        });
    }

    function fetchTelemetry() {
      if (pendingSend) return;
      const t0 = performance.now();
      fetch('/api/telemetry')
        .then(function(r) {
          if (!r.ok) throw new Error('net');
          return r.json();
        })
        .then(function(data) {
          const ping = Math.round(performance.now() - t0);
          pingVal.textContent = ping + ' ms';
          setConnected(true);
          updateTelemetry(data);
        })
        .catch(function() {
          setConnected(false);
        });
    }

    function setConnected(c) {
      if (c === isConnected) return;
      isConnected = c;
      if(c) {
        connDot.classList.add('on');
        connText.textContent = 'ONLINE';
        connText.style.color = 'var(--success)';
      } else {
        connDot.classList.remove('on');
        connText.textContent = 'OFFLINE';
        connText.style.color = 'var(--text-dim)';
      }
    }

    function updateTelemetry(data) {
      if (data.voltage !== undefined) {
        setBattery(data.voltage);
      }
      if (data.temp !== undefined) {
        var tempEl = document.getElementById('tempVal');
        tempEl.textContent = data.temp.toFixed(1) + '\u00B0C';
        // Color: green < 50, orange 50-70, red > 70
        if (data.temp < 50) tempEl.style.color = 'var(--success)';
        else if (data.temp < 70) tempEl.style.color = '#f97316';
        else tempEl.style.color = 'var(--danger)';
      }
    }

    function setBattery(v) {
      battVal.textContent = v.toFixed(1) + ' V';
      // Fill width: full at 12.6V, empty at 10.0V
      const pct = Math.max(0, Math.min(1, (v - 10.0) / (12.6 - 10.0)));
      battFill.setAttribute('width', (23 * pct).toFixed(1));
      // Color
      if (pct > 0.5) battFill.setAttribute('fill', 'var(--success)');
      else if (pct > 0.2) battFill.setAttribute('fill', 'var(--warning)');
      else battFill.setAttribute('fill', 'var(--danger)');
    }


    // ===== INIT =====
    function init() {
      positionThrottleZero();
      updateThrottleUI();
      updateRudderUI();
      // Generate throttle ticks
      var ticksEl = $('throttleTicks');
      var ticks = [100, 75, 50, 25, 0, -25];
      for (var i = 0; i < ticks.length; i++) {
        var s = document.createElement('span');
        s.className = 'throttle-tick';
        s.textContent = ticks[i];
        ticksEl.appendChild(s);
      }
      // Start telemetry polling
      setInterval(fetchTelemetry, 1000);
    }

    // Handle resize
    window.addEventListener('resize', function() {
      updateThrottleUI();
      updateRudderUI();
    });

    // Prevent pull-to-refresh on mobile
    document.addEventListener('touchmove', function(e) {
      if (e.touches.length > 1) e.preventDefault();
    }, {passive: false});

    // Init on load
    if (document.readyState === 'loading') {
      document.addEventListener('DOMContentLoaded', init);
    } else {
      init();
    }
  })();
  </script>
</body>
</html>
)rawliteral";

// Parses the incoming HTTP request and responds accordingly
void processHttpRequest(WiFiClient &client, String req) {

  // Throttle API (also accepts rudder parameter)
  if (req.startsWith("GET /api/throttle")) {
    int vIndex = req.indexOf("v=");
    if (vIndex != -1) {
      int endIndex = req.indexOf("&", vIndex);
      if (endIndex == -1)
        endIndex = req.indexOf(" ", vIndex);
      if (endIndex == -1)
        endIndex = req.length();

      String valStr = req.substring(vIndex + 2, endIndex);
      int val = valStr.toInt();

      // Accept range 640-2000 (includes reverse below 1000)
      if (val >= 640 && val <= 2000) {
        currentThrottle = val;
        esc.writeMicroseconds(currentThrottle);
      }
    }
    // Parse rudder parameter
    int rIndex = req.indexOf("r=");
    if (rIndex != -1) {
      int endIndex = req.indexOf(" ", rIndex);
      if (endIndex == -1)
        endIndex = req.length();
      String rStr = req.substring(rIndex + 2, endIndex);
      int rVal = rStr.toInt();
      if (rVal >= -45 && rVal <= 45) {
        currentRudder = rVal;
        rudder.write(90 + currentRudder); // map -45..45 to 45..135 degrees
      }
    }
  }

  // Calculate speed percentage for UI
  // Forward: 1000-2000 → 0-100%, Reverse: 1000-640 → 0 to -25%
  int speedPct = 0;
  if (currentThrottle >= 1000) {
    speedPct = map(currentThrottle, 1000, 2000, 0, 100);
  } else {
    speedPct = map(currentThrottle, 1000, 640, 0, -25);
  }
  speedPct = constrain(speedPct, -25, 100);

  // Send JSON response for both telemetry & throttle update
  if (req.startsWith("GET /api/")) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type: application/json");
    client.println("Connection: close");
    client.println("Access-Control-Allow-Origin: *"); // Useful for debugging
                                                      // from local file
    client.println();

    client.print("{\"voltage\":");
    client.print(batteryVoltage);
    client.print(",\"temp\":");
    client.print(escTemperature);
    client.print(",\"speed_pct\":");
    client.print(speedPct);
    client.print(",\"throttle\":");
    client.print(currentThrottle);
    client.print(",\"rudder\":");
    client.print(currentRudder);
    client.println("}");
  } else {
    // Default: Serve the HTML page
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type: text/html");
    client.println("Connection: close");
    client.println();
    client.print(index_html);
  }
}

void loop() {
  if (status != WiFi.status()) {
    status = WiFi.status();
    if (status == WL_AP_CONNECTED) {
      Serial.println("Device connected to AP");
    } else {
      Serial.println("Device disconnected from AP");
      // Safety First: STOP motor when device disconnects
      currentThrottle = 1000;
      esc.writeMicroseconds(currentThrottle);
    }
  }

  // Read real battery voltage from analog pin
  int rawADC = analogRead(batteryPin);
  batteryVoltage = (rawADC / 1023.0) * 5.0 * VOLTAGE_DIVIDER_RATIO;

  // Read ESC temperature from NTC thermistor
  int tempADC = analogRead(tempPin);
  if (tempADC > 0 && tempADC < 1023) {
    float resistance = SERIES_RESISTOR / ((1023.0 / tempADC) - 1.0);
    float steinhart = log(resistance / THERMISTOR_NOMINAL) / B_COEFFICIENT;
    steinhart += 1.0 / (TEMP_NOMINAL + 273.15);
    escTemperature = (1.0 / steinhart) - 273.15;
  }

  // Scroll IP on LED matrix every 30 seconds
  if (millis() - lastScrollTime >= 30000) {
    lastScrollTime = millis();
    matrix.beginDraw();
    matrix.stroke(0xFFFFFFFF);
    matrix.textScrollSpeed(80);
    matrix.textFont(Font_5x7);
    matrix.beginText(0, 1, 0xFFFFFF);
    matrix.println(scrollText);
    matrix.endText(SCROLL_LEFT);
    matrix.endDraw();
  }

  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    String requestLine = "";
    bool requestLineRead = false;

    while (client.connected()) {
      // 10 microsecond delay is required for AP stability on Arduino WiFi
      // module
      delayMicroseconds(10);
      if (client.available()) {
        char c = client.read();

        // Read the first line (HTTP Request Line)
        if (c == '\n' && currentLine.length() > 0) {
          if (!requestLineRead) {
            requestLine = currentLine;
            requestLineRead = true;
          }
          currentLine = "";
        } else if (c == '\n' && currentLine.length() == 0) {
          // Empty line indicates end of HTTP headers
          processHttpRequest(client, requestLine);
          break;
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    // Close the connection
    client.stop();
  }
}

void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}
