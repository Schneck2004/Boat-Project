R"html(
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
      grid-template-columns: 1fr 1fr 1fr;
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
    .rudder-container {
      width: 100%;
      margin: 40px 0 20px;
      margin-top: 20px;
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
    <div class="panel">
      <div class="label">Rudder Angle</div>
      <div calss"value" id="valRud">-- º</div>
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
      <div class="label">Rudder Control</div>
      <div class="value" id="rudderDisplay">0º</div>
      <div class="rudder-container">
        <input type="range" id="rudderSlider" min="-45" max="45" step="5" value="0"
              oninput="updateRudderDisplay(this.value); sendRudder();"
              ontouchmove="updateRudderDisplay(this.value); sendRudder();">
        <div class="ticks">
          <span>LEFT</span>
          <span>CENTER</span>
          <span>RIGHT</span>
        </div>
      </div>
    </div>
  </div>

  <script>
    let isConnected = false;
    let pendingThrottleUpdate = false;
    let pendingRudderUpdate = false;

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

    function updateRudderDisplay(val) {
      document.getElementById('rudderDisplay').innerText = val + 'º';
      document.getElementById('rudderDisplay').style.color = '#ffffff';
      document.getElementById('rudderDisplay').style.textShadow = '0 0 10px rgba(255, 255, 255, 0.5)';
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

    function sendRudder() {
      if(pendingRudderUpdate) return;
      pendingRudderUpdate = true;

      const val = document.getElementById('rudderSlider').value;
      fetch(`/api/rudangle?v=${val}`)
        .then(response => {
          if(!response.ok) throw new Error("Network response not ok");
          return response.json();
        })
        .then(data => {
          updateConnStatus(true);
          updateTelemetryUI(data);
          pendingRudderUpdate = false;
        })
        .catch(err => {
          updateConnStatus(false);
          pendingRudderUpdate = false;
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
      if(pendingThrottleUpdate || pendingRudderUpdate) return; // Don't fetch while sending throttle or rudder angle to reduce load on Arduino
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
      if(data.rudangle !== undefined) {
        document.getElementById('valRud').innerText = data.rudangle + ' º';
      }
    }

    // Check status every 1000ms
    setInterval(fetchTelemetry, 1000);
    // Init display
    updateThrottleDisplay(1500);
    updateRudderDisplay(0);
  </script>
</body>
</html>
)html"