#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LoRa.h>
#include <SPI.h>

// ==========================================
//             HARDWARE SETTINGS
// ==========================================
// Check your board! Standard TTGO LoRa32 uses: ss=18, rst=14, dio0=26
// Based on your previous code, you used:
#define ss 5
#define rst 14
#define dio0 2  // If this doesn't work, try 26

// ==========================================
//              NETWORK SETTINGS
// ==========================================
const char* AP_SSID = "EMERGENCY-NODE-01";
const char* AP_PASS = "admin123"; // Password for the WiFi

// ==========================================
//               GLOBAL VARS
// ==========================================
AsyncWebServer server(80);

// Data storage
String lastMsg = "NO DATA";
int lastRSSI = -120; // Default low signal
int packetCount = 0;
float snr = 0;

// Flags for main loop processing
bool shouldSend = false;
String msgToSend = "";

// ==========================================
//            WEB INTERFACE (HTML/JS)
// ==========================================
// Stored in PROGMEM to save RAM. 
// Includes Custom Graph Engine (No Internet Required)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>EMERGENCY NODE</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    :root { --bg: #0d0d0d; --panel: #1a1a1a; --text: #e0e0e0; --accent: #ff3333; --chart-line: #00ff00; }
    body { font-family: "Courier New", monospace; background: var(--bg); color: var(--text); margin: 0; padding: 20px; text-align: center; }
    
    /* LAYOUT */
    .container { max-width: 800px; margin: auto; }
    .header { border-bottom: 2px solid var(--accent); padding-bottom: 10px; margin-bottom: 20px; }
    h1 { color: var(--accent); text-transform: uppercase; letter-spacing: 2px; margin: 0; font-size: 1.5rem; }
    .status-blink { animation: blink 2s infinite; color: #00ff00; font-size: 0.8rem; }
    
    /* DASHBOARD GRID */
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 20px; }
    .card { background: var(--panel); padding: 15px; border-left: 4px solid var(--accent); text-align: left; }
    .card h3 { margin: 0 0 5px 0; font-size: 0.8rem; color: #888; }
    .value { font-size: 1.5rem; font-weight: bold; }
    .small { font-size: 0.8rem; color: #666; }

    /* GRAPH CONTAINER */
    .graph-box { background: var(--panel); padding: 10px; margin-bottom: 20px; border: 1px solid #333; }
    canvas { width: 100%; height: 200px; background: #000; }

    /* TERMINAL LOG */
    .terminal { background: #000; border: 1px solid #333; height: 150px; overflow-y: scroll; padding: 10px; text-align: left; font-size: 0.9rem; margin-bottom: 20px; }
    .log-entry { margin-bottom: 4px; border-bottom: 1px solid #222; }
    .ts { color: #888; margin-right: 10px; }

    /* CONTROLS */
    .input-group { display: flex; gap: 5px; }
    input { flex: 1; padding: 12px; background: #222; border: 1px solid #444; color: white; font-family: monospace; }
    button { padding: 12px 24px; background: var(--accent); color: black; border: none; font-weight: bold; cursor: pointer; }
    button:active { background: #cc0000; }

    @keyframes blink { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }
  </style>
</head>
<body>

<div class="container">
  <div class="header">
    <h1>Emergency Messager</h1>
    <div class="status-blink">SYSTEM ONLINE // LISTENING 433MHz</div>
  </div>

  <div class="grid">
    <div class="card">
      <h3>SIGNAL STRENGTH (RSSI)</h3>
      <div class="value" id="rssi">--</div>
      <span class="small">dBm (Closer to 0 is better)</span>
    </div>
    <div class="card">
      <h3>PACKETS RECEIVED</h3>
      <div class="value" id="count">0</div>
      <span class="small">Total Count</span>
    </div>
  </div>

  <div class="graph-box">
    <h3>LIVE SIGNAL ANALYSIS</h3>
    <canvas id="signalChart"></canvas>
  </div>

  <div class="terminal" id="term">
    <div class="log-entry"> > System Initialized... Waiting for data.</div>
  </div>

  <div class="input-group">
    <input type="text" id="txtMsg" placeholder="BROADCAST ALERT MESSAGE...">
    <button onclick="sendMsg()">TX SEND</button>
  </div>
</div>

<script>
  // --- GRAPH ENGINE (Lightweight, No Libraries) ---
  const canvas = document.getElementById('signalChart');
  const ctx = canvas.getContext('2d');
  let dataPoints = new Array(50).fill(-140); // Store last 50 RSSI values

  function resizeCanvas() {
    canvas.width = canvas.parentElement.offsetWidth;
    canvas.height = 200;
  }
  window.onresize = resizeCanvas;
  resizeCanvas();

  function drawGraph() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    // Draw Grid
    ctx.strokeStyle = '#222';
    ctx.beginPath();
    for(let i=0; i<canvas.height; i+=40) { ctx.moveTo(0, i); ctx.lineTo(canvas.width, i); }
    ctx.stroke();

    // Draw Line
    ctx.strokeStyle = '#00ff00';
    ctx.lineWidth = 2;
    ctx.beginPath();
    
    let step = canvas.width / (dataPoints.length - 1);
    
    // Map RSSI (-140 to -20) to Canvas Height
    for(let i=0; i<dataPoints.length; i++) {
      let val = dataPoints[i];
      // Normalize: -140(bottom) to -20(top)
      let y = canvas.height - ((val + 140) / 120 * canvas.height);
      if (i==0) ctx.moveTo(0, y);
      else ctx.lineTo(i * step, y);
    }
    ctx.stroke();
  }

  // --- DATA FETCHING ---
  let lastPacketCount = -1;

  setInterval(function() {
    fetch("/status").then(response => response.json()).then(data => {
      
      // Update Numbers
      document.getElementById("rssi").innerText = data.rssi;
      document.getElementById("count").innerText = data.packets;

      // Update Graph
      dataPoints.push(data.rssi); 
      dataPoints.shift(); // Remove oldest
      drawGraph();

      // Update Terminal if new packet arrived
      if(data.packets > lastPacketCount && lastPacketCount != -1) {
        logToTerminal(data.last, data.rssi);
      }
      lastPacketCount = data.packets;
    });
  }, 1000); // Refresh every 1 second

  function logToTerminal(msg, rssi) {
    const term = document.getElementById("term");
    const d = new Date();
    const time = d.getHours() + ":" + d.getMinutes() + ":" + d.getSeconds();
    
    const div = document.createElement("div");
    div.className = "log-entry";
    div.innerHTML = `<span class="ts">[${time}]</span> RX: ${msg} <span style="color:#666">(${rssi}dBm)</span>`;
    
    term.prepend(div); // Add to top
  }

  function sendMsg() {
    var msg = document.getElementById("txtMsg").value;
    if(!msg) return;
    
    // Optimistic UI Update
    const term = document.getElementById("term");
    const div = document.createElement("div");
    div.className = "log-entry";
    div.innerHTML = `<span class="ts" style="color:var(--accent)">[TX]</span> SENDING: ${msg}`;
    term.prepend(div);
    document.getElementById("txtMsg").value = "";

    fetch("/send?msg=" + encodeURIComponent(msg));
  }
</script>
</body>
</html>
)rawliteral";

// ==========================================
//               SETUP & LOOP
// ==========================================
void setup() {
  Serial.begin(115200);

  // 1. WiFi Access Point Setup
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("\n==================================");
  Serial.print("  EMERGENCY NODE ACTIVE\n  Connect to: ");
  Serial.println(AP_SSID);
  Serial.print("  Go to URL:  http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("==================================");

  // 2. LoRa Hardware Setup
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6)) {
    Serial.println("[ERROR] LoRa Start Failed! Check Wiring.");
    while (1);
  }
  LoRa.setSyncWord(0xA5); // Security Sync Word
  Serial.println("[OK] LoRa Listening on 433MHz");

  // 3. Web Server Routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){
    r->send(200, "text/html", index_html);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *r){
    String json = "{";
    json += "\"packets\":" + String(packetCount) + ",";
    json += "\"rssi\":" + String(lastRSSI) + ",";
    json += "\"snr\":" + String(snr) + ",";
    json += "\"last\":\"" + lastMsg + "\"}";
    r->send(200, "application/json", json);
  });

  server.on("/send", HTTP_GET, [](AsyncWebServerRequest *r){
    if (r->hasArg("msg")) {
      msgToSend = r->arg("msg");
      shouldSend = true;
    }
    r->send(200, "text/plain", "OK");
  });

  server.begin();
}

void loop() {
  // 1. RECEIVE MODE
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String recv = "";
    while (LoRa.available()) {
      recv += (char)LoRa.read();
    }
    lastRSSI = LoRa.packetRssi();
    snr = LoRa.packetSnr();
    lastMsg = recv;
    packetCount++;
    
    Serial.printf("RX: %s | RSSI: %d\n", recv.c_str(), lastRSSI);
  }

  // 2. TRANSMIT MODE (Safe Handling)
  if (shouldSend) {
    Serial.print("TX: ");
    Serial.println(msgToSend);
    
    LoRa.beginPacket();
    LoRa.print(msgToSend);
    LoRa.endPacket();
    
    LoRa.receive(); // switch back to listen mode immediately
    shouldSend = false;
  }
}