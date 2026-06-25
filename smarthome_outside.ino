#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

// ===== KONFIGURASI =====
const char* ssid     = "花の名";
const char* password = "higanbana";

#define RAIN_SENSOR  14
#define SERVO_PIN    27
#define LAMPU_PIN    26

Servo s1;
WebServer server(80);

// ===== MUTEX =====
SemaphoreHandle_t xMutex;

// ===== STATE (dilindungi Mutex) =====
bool modeOtomatis    = true;
bool servoTerbuka    = false;
bool lampuNyala      = false;
bool lampuModeJadwal = false;

int jadwalNyalaMenit = 18 * 60;  // default 18:00
int jadwalMatiMenit  =  6 * 60;  // default 06:00

int jamBrowser       = 0;
int menitBrowser     = 0;
unsigned long millisSaatSync = 0;

// ===== SERVO (debounce: tidak gerak kalau posisi sama) =====
void gerakServo(bool buka) {
  if (buka == servoTerbuka) return;  // FIX: cegah servo jitter
  servoTerbuka = buka;
  s1.write(buka ? 120 : 0);
}

// ===== JAM SEKARANG =====
void getJamSekarang(int &jam, int &menit) {
  unsigned long selisihMs = millis() - millisSaatSync;
  int totalSync = jamBrowser * 60 + menitBrowser;
  int totalNow  = totalSync + (int)(selisihMs / 60000UL);
  totalNow      = totalNow % 1440;
  jam   = totalNow / 60;
  menit = totalNow % 60;
}

// ===== CEK JADWAL =====
bool didalamJadwal() {
  if (millisSaatSync == 0) return false;
  int jam, menit;
  getJamSekarang(jam, menit);
  int sekarang = jam * 60 + menit;
  int mulai    = jadwalNyalaMenit;
  int selesai  = jadwalMatiMenit;
  if (mulai <= selesai) return sekarang >= mulai && sekarang < selesai;
  else                  return sekarang >= mulai || sekarang < selesai;
}

// ===================================================
// TASK 1 — SENSOR & SERVO (prioritas tertinggi: 3)
// Core 1, jalan tiap 200ms
// ===================================================
void taskSensor(void* param) {
  while (true) {
    if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      if (modeOtomatis) {
        bool hujan = digitalRead(RAIN_SENSOR) == LOW;
        gerakServo(!hujan);  // hujan → tutup, tidak hujan → buka
      }
      xSemaphoreGive(xMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// ===================================================
// TASK 2 — JADWAL LAMPU (prioritas sedang: 2)
// Core 1, cek tiap 10 detik
// ===================================================
void taskJadwal(void* param) {
  while (true) {
    if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      if (lampuModeJadwal) {
        bool harusNyala = didalamJadwal();
        if (harusNyala != lampuNyala) {
          lampuNyala = harusNyala;
          digitalWrite(LAMPU_PIN, lampuNyala ? HIGH : LOW);
          Serial.println(lampuNyala ? "Jadwal: Lampu ON" : "Jadwal: Lampu OFF");
        }
      }
      xSemaphoreGive(xMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(10000));  // cek tiap 10 detik
  }
}

// ===================================================
// TASK 3 — WEB SERVER (prioritas rendah: 1)
// Core 0, yield tiap 10ms agar responsif
// ===================================================
void taskWeb(void* param) {
  while (true) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ===== ENDPOINT: STATUS JSON =====
void handleStatus() {
  char nyalaStr[6], matiStr[6];
  int jam = 0, menit = 0;

  xSemaphoreTake(xMutex, portMAX_DELAY);
  sprintf(nyalaStr, "%02d:%02d", jadwalNyalaMenit / 60, jadwalNyalaMenit % 60);
  sprintf(matiStr,  "%02d:%02d", jadwalMatiMenit  / 60, jadwalMatiMenit  % 60);
  bool _hujan  = digitalRead(RAIN_SENSOR) == LOW;
  bool _servo  = servoTerbuka;
  bool _lampu  = lampuNyala;
  bool _modeA  = modeOtomatis;
  bool _modeL  = lampuModeJadwal;
  bool _synced = millisSaatSync > 0;
  if (_synced) getJamSekarang(jam, menit);
  xSemaphoreGive(xMutex);

  String json = "{";
  json += "\"hujan\":"          + String(_hujan  ? "true" : "false") + ",";
  json += "\"servo\":"          + String(_servo  ? "true" : "false") + ",";
  json += "\"lampu\":"          + String(_lampu  ? "true" : "false") + ",";
  json += "\"modeAtap\":"       + String(_modeA  ? "true" : "false") + ",";
  json += "\"modeLampu\":"      + String(_modeL  ? "true" : "false") + ",";
  json += "\"jadwalNyala\":\"" + String(nyalaStr) + "\",";
  json += "\"jadwalMati\":\""  + String(matiStr)  + "\",";
  json += "\"jamSync\":"        + String(_synced ? "true" : "false") + ",";
  json += "\"jamSekarang\":\""  + String(jam) + ":" + String(menit) + "\"";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// ===== ENDPOINT: SYNC JAM =====
void handleSyncJam() {
  if (server.hasArg("h") && server.hasArg("m")) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    jamBrowser     = server.arg("h").toInt();
    menitBrowser   = server.arg("m").toInt();
    millisSaatSync = millis();
    xSemaphoreGive(xMutex);
    Serial.printf("Jam sync: %02d:%02d\n", jamBrowser, menitBrowser);
  }
  server.send(200, "text/plain", "OK");
}

// ===== ENDPOINT: MODE ATAP =====
void handleMode() {
  if (server.hasArg("v")) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    modeOtomatis = (server.arg("v") == "auto");
    xSemaphoreGive(xMutex);
  }
  server.send(200, "text/plain", "OK");
}

// ===== ENDPOINT: KONTROL SERVO MANUAL =====
void handleServo() {
  if (server.hasArg("v")) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    if (!modeOtomatis) gerakServo(server.arg("v") == "buka");
    xSemaphoreGive(xMutex);
  }
  server.send(200, "text/plain", "OK");
}

// ===== ENDPOINT: KONTROL LAMPU MANUAL =====
void handleLampu() {
  if (server.hasArg("v")) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    if (!lampuModeJadwal) {
      lampuNyala = (server.arg("v") == "on");
      digitalWrite(LAMPU_PIN, lampuNyala ? HIGH : LOW);
    }
    xSemaphoreGive(xMutex);
  }
  server.send(200, "text/plain", "OK");
}

// ===== ENDPOINT: MODE LAMPU =====
void handleLampuMode() {
  if (server.hasArg("v")) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    lampuModeJadwal = (server.arg("v") == "jadwal");
    xSemaphoreGive(xMutex);
  }
  server.send(200, "text/plain", "OK");
}

// ===== ENDPOINT: SIMPAN JADWAL =====
void handleJadwal() {
  xSemaphoreTake(xMutex, portMAX_DELAY);
  if (server.hasArg("nyala")) {
    String s = server.arg("nyala");
    jadwalNyalaMenit = s.substring(0, 2).toInt() * 60 + s.substring(3, 5).toInt();
  }
  if (server.hasArg("mati")) {
    String s = server.arg("mati");
    jadwalMatiMenit = s.substring(0, 2).toInt() * 60 + s.substring(3, 5).toInt();
  }
  xSemaphoreGive(xMutex);
  server.send(200, "text/plain", "OK");
}

// ===== HALAMAN WEB =====
String buatHalaman() {
  char nyalaStr[6], matiStr[6];
  sprintf(nyalaStr, "%02d:%02d", jadwalNyalaMenit / 60, jadwalNyalaMenit % 60);
  sprintf(matiStr,  "%02d:%02d", jadwalMatiMenit  / 60, jadwalMatiMenit  % 60);
  String initNyala = String(nyalaStr);
  String initMati  = String(matiStr);

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
// Jam dari browser
function updateJam() {
  var now = new Date();
  var h = String(now.getHours()).padStart(2,'0');
  var m = String(now.getMinutes()).padStart(2,'0');
  var s = String(now.getSeconds()).padStart(2,'0');
  document.getElementById('jam').textContent = h + ':' + m + ':' + s;
}
setInterval(updateJam, 1000);
updateJam();

// Sync jam ke ESP32 tiap 1 menit
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

void handleRoot() {
  server.send(200, "text/html", buatHalaman());
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  pinMode(RAIN_SENSOR, INPUT);
  pinMode(LAMPU_PIN, OUTPUT);
  digitalWrite(LAMPU_PIN, LOW);

  s1.attach(SERVO_PIN);
  s1.write(0);

  // Buat Mutex SEBELUM task dibuat
  xMutex = xSemaphoreCreateMutex();

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  server.on("/",          handleRoot);
  server.on("/mode",      handleMode);
  server.on("/servo",     handleServo);
  server.on("/lampu",     handleLampu);
  server.on("/lampumode", handleLampuMode);
  server.on("/jadwal",    handleJadwal);
  server.on("/status",    handleStatus);
  server.on("/syncjam",   handleSyncJam);
  server.begin();
  Serial.println("Web server started!");

  // ===== BUAT 3 TASK FREERTOS =====
  // xTaskCreatePinnedToCore(fungsi, nama, stackSize, param, prioritas, handle, core)
  xTaskCreatePinnedToCore(taskSensor, "Sensor", 2048, NULL, 3, NULL, 1);  // Core 1, prioritas 3
  xTaskCreatePinnedToCore(taskJadwal, "Jadwal", 2048, NULL, 2, NULL, 1);  // Core 1, prioritas 2
  xTaskCreatePinnedToCore(taskWeb,    "Web",    8192, NULL, 1, NULL, 0);  // Core 0, prioritas 1
}

// ===== LOOP DITIDURKAN =====
// Semua logic sudah di task masing-masing
void loop() {
  vTaskDelay(portMAX_DELAY);
}