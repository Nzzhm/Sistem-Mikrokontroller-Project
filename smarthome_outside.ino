#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

// ===== CONFIGURATION =====
const char* WIFI_SSID     = "花の名";
const char* WIFI_PASSWORD = "higanbana";

#define RAIN_SENSOR_PIN 14
#define SERVO_PIN       27
#define LAMP_PIN        26

Servo roofServo;
WebServer server(80);

// ===== SYSTEM STATE =====
bool isAutomaticMode  = true;
bool isServoOpen      = false;
bool isLampOn         = false;
bool isLampScheduleMode = false;

// Schedule in minutes from midnight (0-1439)
int scheduleOnMinutes  = 18 * 60;  // Default 18:00
int scheduleOffMinutes = 6  * 60;  // Default 06:00

// Browser time synchronization variables
int browserHour      = 0;
int browserMinute    = 0;
unsigned long lastSyncMillis = 0; 

// ===== SERVO CONTROL =====
void controlServo(bool openRoof) {
  isServoOpen = openRoof;
  roofServo.write(openRoof ? 120 : 0);
}

// ===== TIME CALCULATION =====
void getCurrentTime(int &currentHour, int &currentMinute) {
  if (lastSyncMillis == 0) {
    currentHour = 0;
    currentMinute = 0;
    return;
  }
  unsigned long elapsedMillis = millis() - lastSyncMillis;
  int totalSyncMinutes = (browserHour * 60) + browserMinute;
  int totalCurrentMinutes = totalSyncMinutes + (int)(elapsedMillis / 60000UL);
  
  totalCurrentMinutes = totalCurrentMinutes % 1440; // Wrap 24 hours
  currentHour   = totalCurrentMinutes / 60;
  currentMinute = totalCurrentMinutes % 60;
}

// ===== SCHEDULE CHECKER =====
bool isWithinSchedule() {
  if (lastSyncMillis == 0) return false; 
  
  int hour, minute;
  getCurrentTime(hour, minute);
  int currentMinutes = (hour * 60) + minute;
  int startMinutes   = scheduleOnMinutes;
  int endMinutes     = scheduleOffMinutes;
  
  if (startMinutes <= endMinutes) {
    return (currentMinutes >= startMinutes && currentMinutes < endMinutes);
  } else {
    return (currentMinutes >= startMinutes || currentMinutes < endMinutes);
  }
}

// ===== HTTP ENDPOINT: STATUS JSON =====
void handleStatus() {
  char timeOnStr[6], timeOffStr[6];
  snprintf(timeOnStr, sizeof(timeOnStr), "%02d:%02d", scheduleOnMinutes / 60, scheduleOnMinutes % 60);
  snprintf(timeOffStr, sizeof(timeOffStr), "%02d:%02d", scheduleOffMinutes / 60, scheduleOffMinutes % 60);

  char jsonResponse[300];
  snprintf(jsonResponse, sizeof(jsonResponse),
           "{\"hujan\":%s,\"servo\":%s,\"lampu\":%s,\"modeAtap\":%s,\"modeLampu\":%s,\"jadwalNyala\":\"%s\",\"jadwalMati\":\"%s\",\"jamSync\":%s}",
           (digitalRead(RAIN_SENSOR_PIN) == LOW) ? "true" : "false",
           isServoOpen ? "true" : "false",
           isLampOn ? "true" : "false",
           isAutomaticMode ? "true" : "false",
           isLampScheduleMode ? "true" : "false",
           timeOnStr, timeOffStr,
           (lastSyncMillis > 0) ? "true" : "false");

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", jsonResponse);
}

// ===== HTTP ENDPOINT: TIME SYNC =====
void handleTimeSync() {
  if (server.hasArg("h") && server.hasArg("m")) {
    browserHour    = server.arg("h").toInt();
    browserMinute  = server.arg("m").toInt();
    lastSyncMillis = millis();
    Serial.printf("Time synchronized from browser: %02d:%02d\n", browserHour, browserMinute);
  }
  server.send(200, "text/plain", "OK");
}

// ===== WEB INTERFACE PAGE =====
String buildWebPage() {
  char timeOnStr[6], timeOffStr[6];
  snprintf(timeOnStr, sizeof(timeOnStr), "%02d:%02d", scheduleOnMinutes / 60, scheduleOnMinutes % 60);
  snprintf(timeOffStr, sizeof(timeOffStr), "%02d:%02d", scheduleOffMinutes / 60, scheduleOffMinutes % 60);
  
  String initNyala = String(timeOnStr);
  String initMati  = String(timeOffStr);

  String html = R"rawliteral(<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Kontrol Atap dan Lampu</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: 'Segoe UI', sans-serif; background: #f0f4f8; color: #333; }
  .container { max-width: 480px; margin: 24px auto; padding: 16px; }
  h1 { text-align: center; margin-bottom: 4px; color: #1a237e; font-size: 1.3rem; }
  .clock { text-align: center; font-size: 2rem; font-weight: 700; color: #1a237e; margin-bottom: 20px; letter-spacing: 2px; }
  .card { background: white; border-radius: 12px; padding: 16px 20px; margin-bottom: 14px; box-shadow: 0 2px 8px rgba(0,0,0,0.08); }
  .card h2 { font-size: 0.8rem; color: #888; margin-bottom: 10px; text-transform: uppercase; letter-spacing: 1px; }
  .status-row { display: flex; justify-content: space-between; align-items: center; margin: 6px 0; font-size: 0.95rem; }
  .badge { padding: 3px 12px; border-radius: 20px; font-size: 0.82rem; font-weight: 600; color: white; }
  .badge-green  { background: #4CAF50; }
  .badge-orange { background: #FF9800; }
  .badge-blue   { background: #2196F3; }
  .badge-purple { background: #9C27B0; }
  .btn-row { display: flex; gap: 10px; margin-top: 10px; }
  .btn { flex: 1; padding: 11px; border: none; border-radius: 8px; font-size: 0.92rem; font-weight: 600; cursor: pointer; transition: opacity 0.2s; }
  .btn:hover { opacity: 0.85; }
  .btn:disabled { opacity: 0.35; cursor: default; }
  .btn-green  { background: #4CAF50; color: white; }
  .btn-blue   { background: #2196F3; color: white; }
  .btn-orange { background: #FF9800; color: white; }
  .btn-red    { background: #f44336; color: white; }
  .btn-gray   { background: #9E9E9E; color: white; }
  .btn-yellow { background: #FFC107; color: #333; }
  .btn-purple { background: #9C27B0; color: white; }
  .jadwal-row { display: flex; align-items: center; gap: 10px; margin-bottom: 10px; font-size: 0.9rem; }
  .jadwal-row label { width: 80px; color: #555; }
  .jadwal-row input[type=time] { flex: 1; padding: 8px; border: 1px solid #ddd; border-radius: 8px; font-size: 0.95rem; }
  .btn-save { width: 100%; padding: 11px; background: #9C27B0; color: white; border: none; border-radius: 8px; font-weight: 600; font-size: 0.95rem; cursor: pointer; margin-top: 4px; }
  .btn-save:hover { opacity: 0.88; }
  .note { font-size: 0.78rem; color: #999; margin-top: 8px; text-align: center; min-height: 16px; }
  .divider { border: none; border-top: 1px solid #eee; margin: 10px 0; }
</style>
</head>
<body>
<div class="container">
  <h1>Kontrol Atap dan Lampu</h1>
  <div class="clock" id="jam">--:--:--</div>

  <div class="card">
    <h2>Status Saat Ini</h2>
    <div class="status-row"><span>Sensor Hujan</span><strong id="sHujan">-</strong></div>
    <div class="status-row"><span>Atap</span><strong id="sServo">-</strong></div>
    <div class="status-row"><span>Lampu</span><strong id="sLampu">-</strong></div>
    <div class="status-row"><span>Mode Atap</span><span class="badge" id="badgeAtap">-</span></div>
    <div class="status-row"><span>Mode Lampu</span><span class="badge" id="badgeLampu">-</span></div>
  </div>

  <div class="card">
    <h2>Kontrol Atap</h2>
    <div class="btn-row">
      <button class="btn btn-green"  onclick="kirim('/mode?v=auto')">Otomatis</button>
      <button class="btn btn-orange" onclick="kirim('/mode?v=manual')">Manual</button>
    </div>
    <hr class="divider">
    <div class="btn-row">
      <button class="btn btn-blue" id="btnBuka"  onclick="kirim('/servo?v=buka')">Buka</button>
      <button class="btn btn-gray" id="btnTutup" onclick="kirim('/servo?v=tutup')">Tutup</button>
    </div>
    <p class="note" id="noteAtap"></p>
  </div>

  <div class="card">
    <h2>Kontrol Lampu</h2>
    <div class="btn-row">
      <button class="btn btn-blue"   onclick="kirim('/lampumode?v=manual')">Manual</button>
      <button class="btn btn-purple" onclick="kirim('/lampumode?v=jadwal')">Jadwal</button>
    </div>
    <hr class="divider">
    <div class="btn-row">
      <button class="btn btn-yellow" id="btnOn"  onclick="kirim('/lampu?v=on')">Lampu ON</button>
      <button class="btn btn-red"    id="btnOff" onclick="kirim('/lampu?v=off')">Lampu OFF</button>
    </div>
    <p class="note" id="noteLampu"></p>
    <hr class="divider">
    <h2 style="margin-top:4px">Atur Jadwal Lampu</h2>
    <div style="margin-top:12px">
      <div class="jadwal-row">
        <label>Nyala</label>
        <input type="time" id="jamNyala" value=")rawliteral" + initNyala + R"rawliteral(">
      </div>
      <div class="jadwal-row">
        <label>Mati</label>
        <input type="time" id="jamMati" value=")rawliteral" + initMati + R"rawliteral(">
      </div>
      <button class="btn-save" onclick="simpanJadwal()">Simpan Jadwal</button>
      <p class="note">Jadwal aktif hanya jika mode Jadwal dipilih</p>
    </div>
  </div>
</div>

<script>
function updateJam() {
  var now = new Date();
  var h = String(now.getHours()).padStart(2,'0');
  var m = String(now.getMinutes()).padStart(2,'0');
  var s = String(now.getSeconds()).padStart(2,'0');
  document.getElementById('jam').textContent = h + ':' + m + ':' + s;
}
setInterval(updateJam, 1000);
updateJam();

function syncJam() {
  var now = new Date();
  fetch('/syncjam?h=' + now.getHours() + '&m=' + now.getMinutes());
}
syncJam();
setInterval(syncJam, 60000);

function kirim(url) {
  fetch(url).then(function() { updateStatus(); });
}

function simpanJadwal() {
  var nyala = document.getElementById('jamNyala').value;
  var mati  = document.getElementById('jamMati').value;
  if (!nyala || !mati) { alert('Isi jam nyala dan mati dulu!'); return; }
  var now = new Date();
  fetch('/syncjam?h=' + now.getHours() + '&m=' + now.getMinutes())
    .then(function() {
      return fetch('/jadwal?nyala=' + nyala + '&mati=' + mati);
    })
    .then(function() { alert('Jadwal tersimpan!'); updateStatus(); });
}

function updateStatus() {
  fetch('/status')
    .then(function(r) { return r.json(); })
    .then(function(d) {
      document.getElementById('sHujan').textContent = d.hujan ? 'Hujan' : 'Tidak Hujan';
      document.getElementById('sServo').textContent = d.servo ? 'Terbuka (120 deg)' : 'Tertutup (0 deg)';
      document.getElementById('sLampu').textContent = d.lampu ? 'ON' : 'OFF';

      var ba = document.getElementById('badgeAtap');
      ba.textContent = d.modeAtap ? 'Otomatis' : 'Manual';
      ba.className   = 'badge ' + (d.modeAtap ? 'badge-green' : 'badge-orange');

      var bl = document.getElementById('badgeLampu');
      bl.textContent = d.modeLampu ? 'Jadwal' : 'Manual';
      bl.className   = 'badge ' + (d.modeLampu ? 'badge-purple' : 'badge-blue');

      document.getElementById('btnBuka').disabled  = d.modeAtap;
      document.getElementById('btnTutup').disabled = d.modeAtap;
      document.getElementById('noteAtap').textContent = d.modeAtap ? 'Ganti ke Manual untuk kontrol atap' : '';

      document.getElementById('btnOn').disabled  = d.modeLampu;
      document.getElementById('btnOff').disabled = d.modeLampu;
      document.getElementById('noteLampu').textContent = d.modeLampu ? 'Lampu dikontrol oleh jadwal' : '';

      document.getElementById('jamNyala').value = d.jadwalNyala;
      document.getElementById('jamMati').value  = d.jadwalMati;
    })
    .catch(function() {
      document.getElementById('sHujan').textContent = 'Koneksi terputus...';
    });
}

updateStatus();
setInterval(updateStatus, 3000);
</script>
</body>
</html>)rawliteral";

  return html;
}

// ===== ROUTE HANDLERS =====
void handleRoot() {
  server.send(200, "text/html", buildWebPage());
}

void handleMode() {
  if (server.hasArg("v")) isAutomaticMode = (server.arg("v") == "auto");
  server.send(200, "text/plain", "OK");
}

void handleServo() {
  if (server.hasArg("v") && !isAutomaticMode) {
    controlServo(server.arg("v") == "buka");
  }
  server.send(200, "text/plain", "OK");
}

void handleLampu() {
  if (server.hasArg("v") && !isLampScheduleMode) {
    isLampOn = (server.arg("v") == "on");
    digitalWrite(LAMP_PIN, isLampOn ? HIGH : LOW);
  }
  server.send(200, "text/plain", "OK");
}

void handleLampuMode() {
  if (server.hasArg("v")) isLampScheduleMode = (server.arg("v") == "jadwal");
  server.send(200, "text/plain", "OK");
}

void handleJadwal() {
  if (server.hasArg("nyala")) {
    String s = server.arg("nyala");
    int j = s.substring(0, 2).toInt();
    int m = s.substring(3, 5).toInt();
    scheduleOnMinutes = j * 60 + m;
  }
  if (server.hasArg("mati")) {
    String s = server.arg("mati");
    int j = s.substring(0, 2).toInt();
    int m = s.substring(3, 5).toInt();
    scheduleOffMinutes = j * 60 + m;
  }
  server.send(200, "text/plain", "OK");
}

// ===== INITIAL SETUP =====
void setup() {
  Serial.begin(115200);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(LAMP_PIN, OUTPUT);
  digitalWrite(LAMP_PIN, LOW);

  roofServo.attach(SERVO_PIN);
  roofServo.write(0);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println("\nConnected! IP Address: " + WiFi.localIP().toString());

  // Web Server Routing
  server.on("/",          handleRoot);
  server.on("/mode",      handleMode);
  server.on("/servo",     handleServo);
  server.on("/lampu",     handleLampu);
  server.on("/lampumode", handleLampuMode);
  server.on("/jadwal",    handleJadwal);
  server.on("/status",    handleStatus);
  server.on("/syncjam",   handleTimeSync);
  
  server.begin();
  Serial.println("Web server active!");
}

// ===== MAIN LOOP =====
void loop() {
  server.handleClient();

  // Non-blocking Rain Sensor Logic (Every 1 second)
  static unsigned long lastRainCheck = 0;
  if (isAutomaticMode && (millis() - lastRainCheck > 1000)) {
    lastRainCheck = millis();
    int rainSensorValue = digitalRead(RAIN_SENSOR_PIN);
    controlServo(rainSensorValue != LOW);
  }

  // Non-blocking Schedule Logic (Every 30 seconds)
  static unsigned long lastScheduleCheck = 0;
  if (isLampScheduleMode && (millis() - lastScheduleCheck > 30000)) {
    lastScheduleCheck = millis();
    bool shouldBeOn = isWithinSchedule();
    if (shouldBeOn != isLampOn) {
      isLampOn = shouldBeOn;
      digitalWrite(LAMP_PIN, isLampOn ? HIGH : LOW);
      Serial.println(isLampOn ? "Schedule Alert: Lamp turned ON" : "Schedule Alert: Lamp turned OFF");
    }
  }

  delay(10); // Reduced delay for better web server responsiveness
}