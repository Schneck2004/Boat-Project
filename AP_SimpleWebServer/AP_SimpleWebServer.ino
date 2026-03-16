/*
  WiFi Web Server Brushless Motor Control (ESC)

  A simple web server that controls a Brushless Motor via an ESC.
  This sketch creates an access point and launches a web server.
  
  Wiring Instructions:
  - Connect the ESC's signal wire (usually white or yellow) to pin 9.
  - Connect the ESC's BEC 5V wire (usually red) to the Arduino's 5V pin.
  - Connect the ESC's BEC GND wire (usually black or brown) to the Arduino's GND pin.
  - **WARNING**: If you are powering the Arduino via USB while testing, it is 
    recommended to disconnect the ESC's BEC 5V power wire to avoid conflicting power sources.
*/

#include "WiFiS3.h"
#include <Servo.h>
#include "arduino_secrets.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password
int keyIndex = 0;

int led = LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);

void printWiFiStatus();

// ESC Setup
Servo esc;
const int escPin = 9;
int currentThrottle = 1500; // 1500us is neutral (stopped) for most car/boat ESCs

// Simulated Telemetry (Can be replaced with actual sensor reads)
float batteryVoltage = 12.6; 
int engineRPM = 0;

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect
  }
  Serial.println("Access Point Web Server for ESC");

  pinMode(led, OUTPUT);

  // Attach ESC
  esc.attach(escPin);
  esc.writeMicroseconds(currentThrottle); // Send neutral signal to initialize/arm ESC

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
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
    while (true);
  }

  // wait 10 seconds for connection:
  delay(10000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();
}

// Modern Glassmorphism Dashboard UI
const char index_html[] = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Boat Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@500;700&family=Inter:wght@400;600&display=swap" rel="stylesheet">
  <style>
    :root {
      --glass-bg: rgba(255, 255, 255, 0.1);
      --glass-border: rgba(255, 255, 255, 0.2);
      --glass-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.37);
      --accent: #ffffff;
      --text: #ffffff;
      --text-muted: rgba(255, 255, 255, 0.7);
      --danger: #ff4d4d;
      --success: #00ff88;
    }
    body {
      margin: 0;
      padding: 0;
      font-family: 'Inter', sans-serif;
      background-color: #686A6C; /* Nardo Grey */
      color: var(--text);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      overflow-x: hidden;
    }
    .header {
      width: 100%;
      padding: 20px 0;
      text-align: center;
      background: rgba(255, 255, 255, 0.05);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border-bottom: 1px solid var(--glass-border);
      position: sticky;
      top: 0;
      z-index: 10;
      box-shadow: var(--glass-shadow);
    }
    .header h1 {
      margin: 0;
      font-family: 'Orbitron', sans-serif;
      font-weight: 700;
      font-size: 1.8rem;
      letter-spacing: 2px;
      text-transform: uppercase;
      color: var(--text);
    }
    .status-badge {
      display: inline-flex;
      align-items: center;
      gap: 10px;
      font-size: 0.85rem;
      margin-top: 10px;
      padding: 6px 16px;
      background: var(--glass-bg);
      border: 1px solid var(--glass-border);
      border-radius: 30px;
      text-transform: uppercase;
      letter-spacing: 1px;
    }
    .dot {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: #ccc;
      box-shadow: 0 0 5px rgba(0,0,0,0.2);
      transition: all 0.3s;
    }
    .dot.connected {
      background: var(--success);
      box-shadow: 0 0 10px var(--success);
    }
    .dashboard {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 20px;
      padding: 30px 20px;
      width: 100%;
      max-width: 800px;
      box-sizing: border-box;
    }
    .panel {
      background: var(--glass-bg);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border: 1px solid var(--glass-border);
      border-radius: 20px;
      padding: 25px;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      box-shadow: var(--glass-shadow);
    }
    .panel.full-width {
      grid-column: 1 / -1;
    }
    .value {
      font-family: 'Orbitron', sans-serif;
      font-size: 3rem;
      font-weight: 700;
      margin: 15px 0 5px;
      color: var(--accent);
      letter-spacing: 2px;
    }
    .label {
      font-size: 0.95rem;
      color: var(--text-muted);
      text-transform: uppercase;
      letter-spacing: 2px;
      font-weight: 600;
    }
    .throttle-container {
      width: 100%;
      margin: 40px 0 20px;
      position: relative;
    }
    input[type=range] {
      -webkit-appearance: none;
      width: 100%;
      background: transparent;
    }
    input[type=range]:focus {
      outline: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      height: 50px;
      width: 50px;
      border-radius: 50%;
      background: rgba(255, 255, 255, 0.2);
      backdrop-filter: blur(5px);
      cursor: pointer;
      margin-top: -20px;
      border: 2px solid rgba(255, 255, 255, 0.5);
      box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
      position: relative;
      z-index: 2;
      transition: transform 0.1s;
    }
    input[type=range]::-webkit-slider-thumb:active {
      transform: scale(1.1);
      background: rgba(255, 255, 255, 0.3);
    }
    input[type=range]::-webkit-slider-runnable-track {
      width: 100%;
      height: 10px;
      cursor: pointer;
      background: rgba(255,255,255,0.1);
      border-radius: 5px;
      border: 1px solid rgba(255,255,255,0.2);
      box-shadow: inset 0 1px 3px rgba(0,0,0,0.3);
    }
    .ticks {
      display: flex;
      justify-content: space-between;
      padding: 0 15px;
      margin-top: 10px;
      color: var(--text-muted);
      font-size: 0.8rem;
      font-family: 'Orbitron', sans-serif;
    }
    .stop-btn {
      background: rgba(255, 0, 0, 0.25);
      border: 1px solid rgba(255, 0, 0, 0.4);
      color: white;
      padding: 18px 40px;
      font-size: 1.2rem;
      border-radius: 50px;
      cursor: pointer;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 3px;
      transition: all 0.3s ease;
      backdrop-filter: blur(5px);
      width: 100%;
      max-width: 350px;
      margin-top: 20px;
      box-shadow: 0 0 10px rgba(255, 0, 0, 0.1);
    }
    .stop-btn:hover {
      background: rgba(255, 0, 0, 0.4);
      box-shadow: 0 0 20px rgba(255, 0, 0, 0.3);
      transform: translateY(-2px);
    }
    .stop-btn:active {
      transform: translateY(1px);
    }
    #valRPM {
      color: var(--text);
    }
    
    @media (max-width: 600px) {
      .dashboard {
        grid-template-columns: 1fr;
      }
      .value {
        font-size: 2.5rem;
      }
    }
  </style>
</head>
<body>
  <div class="header">
    <h1>Boat Dashboard</h1>
    <div class="status-badge">
      <div id="connDot" class="dot"></div>
      <span id="connText">Connecting...</span>
    </div>
  </div>

  <div class="dashboard">
    <div class="panel">
      <div class="label">System Volts</div>
      <div class="value" id="valVolt">-- V</div>
    </div>
    <div class="panel">
      <div class="label">Engine Power</div>
      <div class="value" id="valRPM">-- %</div>
    </div>
    
    <div class="panel full-width">
      <div class="label">Throttle Control</div>
      <div class="value" id="throttleDisplay">0%</div>
      <div class="throttle-container">
        <!-- Range from 1000us to 2000us, step 10, neutral is 1500 -->
        <input type="range" id="throttleSlider" min="1000" max="2000" step="10" value="1500" 
               oninput="updateThrottleDisplay(this.value); sendThrottle();" 
               ontouchmove="updateThrottleDisplay(this.value); sendThrottle();">
        <div class="ticks">
          <span>REV</span>
          <span>NEUTRAL</span>
          <span>FWD</span>
        </div>
      </div>
      <button class="stop-btn" onclick="emergencyStop()">E-STOP / NEUTRAL</button>
    </div>
  </div>

  <script>
    let isConnected = false;
    let pendingThrottleUpdate = false;

    function updateConnStatus(connected) {
      const dot = document.getElementById('connDot');
      const text = document.getElementById('connText');
      if (connected) {
        if(!isConnected) {
            dot.className = 'dot connected';
            text.innerText = 'Online';
        }
      } else {
        if(isConnected) {
            dot.className = 'dot';
            text.innerText = 'Offline';
        }
      }
      isConnected = connected;
    }

    function updateThrottleDisplay(val) {
      let percent = 0;
      if (val >= 1500) {
        percent = Math.round(((val - 1500) / 500) * 100);
      } else {
        percent = Math.round(((1500 - val) / 500) * -100);
      }
      document.getElementById('throttleDisplay').innerText = percent + '%';
      
      // Change color based on direction
      if(val > 1500) {
        document.getElementById('throttleDisplay').style.color = 'var(--accent)';
        document.getElementById('throttleDisplay').style.textShadow = '0 0 20px rgba(0, 240, 255, 0.5)';
      } else if (val < 1500) {
        document.getElementById('throttleDisplay').style.color = 'var(--danger)';
        document.getElementById('throttleDisplay').style.textShadow = '0 0 20px rgba(255, 42, 42, 0.5)';
      } else {
        document.getElementById('throttleDisplay').style.color = '#ffffff';
        document.getElementById('throttleDisplay').style.textShadow = '0 0 10px rgba(255, 255, 255, 0.5)';
      }
    }

    function sendThrottle() {
      // Avoid overlapping requests by dropping rapid changes (optional depending on ESP capabilities)
      if(pendingThrottleUpdate) return;
      pendingThrottleUpdate = true;
      
      const val = document.getElementById('throttleSlider').value;
      fetch(`/api/throttle?v=${val}`)
        .then(response => {
          if(!response.ok) throw new Error("Network response not ok");
          return response.json();
        })
        .then(data => {
          updateConnStatus(true);
          updateTelemetryUI(data);
          pendingThrottleUpdate = false;
        })
        .catch(err => {
          updateConnStatus(false);
          pendingThrottleUpdate = false;
        });
    }

    function emergencyStop() {
      const slider = document.getElementById('throttleSlider');
      slider.value = 1500;
      updateThrottleDisplay(1500);
      
      fetch(`/api/throttle?v=1500`)
        .then(response => response.json())
        .then(data => {
          updateConnStatus(true);
          updateTelemetryUI(data);
        })
        .catch(err => updateConnStatus(false));
    }

    function fetchTelemetry() {
      if(pendingThrottleUpdate) return; // Don't fetch while sending throttle to reduce load on Arduino
      fetch('/api/telemetry')
        .then(r => {
          if(!r.ok) throw new Error("Network response not ok");
          return r.json();
        })
        .then(data => {
          updateConnStatus(true);
          updateTelemetryUI(data);
        })
        .catch(err => updateConnStatus(false));
    }

    function updateTelemetryUI(data) {
      if(data.voltage !== undefined) {
         document.getElementById('valVolt').innerText = data.voltage.toFixed(1) + ' V';
      }
      if(data.speed_pct !== undefined) {
         let p = data.speed_pct;
         document.getElementById('valRPM').innerText = Math.abs(p) + ' %';
      }
    }

    // Check status every 1000ms
    setInterval(fetchTelemetry, 1000);
    // Init display
    updateThrottleDisplay(1500);
  </script>
</body>
</html>
)rawliteral";


// Parses the incoming HTTP request and responds accordingly
void processHttpRequest(WiFiClient& client, String req) {
  
  // Throttle API
  if (req.startsWith("GET /api/throttle")) {
    int vIndex = req.indexOf("v=");
    if (vIndex != -1) {
      int endIndex = req.indexOf(" ", vIndex);
      if (endIndex == -1) endIndex = req.length();
      
      String valStr = req.substring(vIndex + 2, endIndex);
      int val = valStr.toInt();
      
      // Strict constraint for servo microsecond range 1000-2000
      if (val >= 1000 && val <= 2000) {
        currentThrottle = val;
        esc.writeMicroseconds(currentThrottle);
      }
    }
  }

  // Calculate speed percentage for UI
  int speedPct = 0;
  if(currentThrottle >= 1500) {
    speedPct = map(currentThrottle, 1500, 2000, 0, 100);
  } else {
    speedPct = -map(currentThrottle, 1500, 1000, 0, 100);
  }

  // Send JSON response for both telemetry & throttle update
  if(req.startsWith("GET /api/")) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type: application/json");
      client.println("Connection: close");
      client.println("Access-Control-Allow-Origin: *"); // Useful for debugging from local file
      client.println();
      
      client.print("{\"voltage\":");
      client.print(batteryVoltage);
      client.print(",\"speed_pct\":");
      client.print(speedPct);
      client.print(",\"throttle\":");
      client.print(currentThrottle);
      client.println("}");
  } 
  else {
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
      // Safety First: Neutral throttle when device disconnects
      currentThrottle = 1500;
      esc.writeMicroseconds(currentThrottle);
    }
  }

  // Simulated Telemetry logic (e.g., voltage slightly sags as throttle increases)
  int absThrottle = abs(currentThrottle - 1500);
  batteryVoltage = 12.6 - (absThrottle * 0.001); 

  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    String requestLine = "";
    bool requestLineRead = false;

    while (client.connected()) {
      // 10 microsecond delay is required for AP stability on Arduino WiFi module
      delayMicroseconds(10); 
      if (client.available()) {
        char c = client.read();

        // Read the first line (HTTP Request Line)
        if (c == '\n' && currentLine.length() > 0) {
            if(!requestLineRead) {
                requestLine = currentLine;
                requestLineRead = true;
            }
            currentLine = "";
        } 
        else if (c == '\n' && currentLine.length() == 0) {
          // Empty line indicates end of HTTP headers
          processHttpRequest(client, requestLine);
          break;
        } 
        else if (c != '\r') {
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
