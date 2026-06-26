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

// ===== STATE =====
bool modeOtomatis    = true;
bool servoTerbuka    = false;
bool lampuNyala      = false;
bool lampuModeJadwal = false;

int jadwalNyalaMenit = 18 * 60;
int jadwalMatiMenit  =  6 * 60;

int jamBrowser       = 0;
int menitBrowser     = 0;
unsigned long millisSaatSync = 0;

// ===== SERVO (debounce) =====
void gerakServo(bool buka) {
  if (buka == servoTerbuka) return;
  servoTerbuka = buka;
  s1.write(buka ? 120 : 0);
}

// ===== JAM =====
void getJamSekarang(int &jam, int &menit) {
  unsigned long selisihMs = millis() - millisSaatSync;
  int totalSync = jamBrowser * 60 + menitBrowser;
  int totalNow  = totalSync + (int)(selisihMs / 60000UL);
  totalNow      = totalNow % 1440;
  jam   = totalNow / 60;
  menit = totalNow % 60;
}

bool didalamJadwal() {
  if (millisSaatSync == 0) return false;
  int jam, menit;
  getJamSekarang(jam, menit);
  int sekarang = jam * 60 + menit;
  if (jadwalNyalaMenit <= jadwalMatiMenit)
    return sekarang >= jadwalNyalaMenit && sekarang < jadwalMatiMenit;
  else
    return sekarang >= jadwalNyalaMenit || sekarang < jadwalMatiMenit;
}

// ===================================================
// TASK 1 — SENSOR & SERVO (prioritas 3, Core 1)
// ===================================================
void taskSensor(void* param) {
  while (true) {
    if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      if (modeOtomatis) {
        bool hujan = digitalRead(RAIN_SENSOR) == LOW;
        gerakServo(!hujan);
      }
      xSemaphoreGive(xMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// ===================================================
// TASK 2 — JADWAL LAMPU (prioritas 2, Core 1)
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
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}

// ===================================================
// TASK 3 — WEB SERVER (prioritas 1, Core 0)
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

  char buf[320];
  snprintf(buf, sizeof(buf),
    "{\"hujan\":%s,\"servo\":%s,\"lampu\":%s,\"modeAtap\":%s,"
    "\"modeLampu\":%s,\"jadwalNyala\":\"%s\",\"jadwalMati\":\"%s\","
    "\"jamSync\":%s,\"jam\":%d,\"menit\":%d}",
    _hujan  ? "true":"false",
    _servo  ? "true":"false",
    _lampu  ? "true":"false",
    _modeA  ? "true":"false",
    _modeL  ? "true":"false",
    nyalaStr, matiStr,
    _synced ? "true":"false",
    jam, menit
  );

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", buf);
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

void handleMode() {
  if (server.hasArg("v")) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    modeOtomatis = (server.arg("v") == "auto");
    xSemaphoreGive(xMutex);
  }
  server.send(200, "text/plain", "OK");
}

void handleServo() {
  if (server.hasArg("v")) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    if (!modeOtomatis) gerakServo(server.arg("v") == "buka");
    xSemaphoreGive(xMutex);
  }
  server.send(200, "text/plain", "OK");
}

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

void handleLampuMode() {
  if (server.hasArg("v")) {
    xSemaphoreTake(xMutex, portMAX_DELAY);
    lampuModeJadwal = (server.arg("v") == "jadwal");
    xSemaphoreGive(xMutex);
  }
  server.send(200, "text/plain", "OK");
}

void handleJadwal() {
  xSemaphoreTake(xMutex, portMAX_DELAY);
  if (server.hasArg("nyala")) {
    String s = server.arg("nyala");
    jadwalNyalaMenit = s.substring(0,2).toInt()*60 + s.substring(3,5).toInt();
  }
  if (server.hasArg("mati")) {
    String s = server.arg("mati");
    jadwalMatiMenit = s.substring(0,2).toInt()*60 + s.substring(3,5).toInt();
  }
  xSemaphoreGive(xMutex);
  server.send(200, "text/plain", "OK");
}

// ===== HALAMAN WEB =====
// HTML disimpan di Flash (PROGMEM) bukan RAM → lebih hemat memori
const char HTML_PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Smart Home</title>
<style>
  :root {
    --bg: #0f172a;
    --surface: #1e293b;
    --surface2: #273548;
    --border: #334155;
    --text: #e2e8f0;
    --text2: #94a3b8;
    --green: #22c55e;
    --blue: #3b82f6;
    --orange: #f97316;
    --red: #ef4444;
    --purple: #a855f7;
    --yellow: #eab308;
    --radius: 14px;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: 'Segoe UI', sans-serif; background: var(--bg); color: var(--text); min-height: 100vh; }
  .container { max-width: 460px; margin: 0 auto; padding: 20px 16px 40px; }

  /* Header */
  .header { text-align: center; margin-bottom: 24px; }
  .header h1 { font-size: 1.1rem; color: var(--text2); font-weight: 500; letter-spacing: 2px; text-transform: uppercase; margin-bottom: 8px; }
  .clock { font-size: 3rem; font-weight: 700; color: var(--text); letter-spacing: 4px; font-variant-numeric: tabular-nums; }
  .dot-pulse { display: inline-block; width: 8px; height: 8px; border-radius: 50%; background: var(--green); margin-left: 8px; vertical-align: middle; animation: pulse 2s infinite; }
  @keyframes pulse { 0%,100%{opacity:1;transform:scale(1)} 50%{opacity:.4;transform:scale(.8)} }

  /* Cards */
  .card { background: var(--surface); border: 1px solid var(--border); border-radius: var(--radius); padding: 18px; margin-bottom: 12px; }
  .card-title { font-size: 0.7rem; color: var(--text2); text-transform: uppercase; letter-spacing: 1.5px; margin-bottom: 14px; font-weight: 600; }

  /* Status grid */
  .status-grid { display: grid; gap: 8px; }
  .status-item { display: flex; justify-content: space-between; align-items: center; padding: 10px 12px; background: var(--surface2); border-radius: 10px; }
  .status-label { font-size: 0.85rem; color: var(--text2); }
  .status-val { font-size: 0.85rem; font-weight: 600; }

  /* Badge */
  .badge { padding: 3px 10px; border-radius: 20px; font-size: 0.75rem; font-weight: 600; }
  .badge-green  { background: rgba(34,197,94,.15);  color: var(--green); }
  .badge-orange { background: rgba(249,115,22,.15); color: var(--orange); }
  .badge-blue   { background: rgba(59,130,246,.15); color: var(--blue); }
  .badge-purple { background: rgba(168,85,247,.15); color: var(--purple); }
  .badge-red    { background: rgba(239,68,68,.15);  color: var(--red); }

  /* Icon dot */
  .dot { width: 8px; height: 8px; border-radius: 50%; display: inline-block; margin-right: 6px; }
  .dot-green  { background: var(--green); box-shadow: 0 0 6px var(--green); }
  .dot-red    { background: var(--red);   box-shadow: 0 0 6px var(--red); }
  .dot-blue   { background: var(--blue);  box-shadow: 0 0 6px var(--blue); }
  .dot-gray   { background: var(--text2); }

  /* Buttons */
  .btn-group { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-top: 12px; }
  .btn { padding: 12px; border: none; border-radius: 10px; font-size: 0.88rem; font-weight: 600; cursor: pointer; transition: all .15s; letter-spacing: .3px; }
  .btn:hover { filter: brightness(1.15); transform: translateY(-1px); }
  .btn:active { transform: translateY(0); filter: brightness(.9); }
  .btn:disabled { opacity: .3; cursor: default; transform: none; filter: none; }
  .btn-green  { background: rgba(34,197,94,.2);  color: var(--green);  border: 1px solid rgba(34,197,94,.3); }
  .btn-blue   { background: rgba(59,130,246,.2); color: var(--blue);   border: 1px solid rgba(59,130,246,.3); }
  .btn-orange { background: rgba(249,115,22,.2); color: var(--orange); border: 1px solid rgba(249,115,22,.3); }
  .btn-red    { background: rgba(239,68,68,.2);  color: var(--red);    border: 1px solid rgba(239,68,68,.3); }
  .btn-gray   { background: rgba(148,163,184,.1);color: var(--text2);  border: 1px solid var(--border); }
  .btn-yellow { background: rgba(234,179,8,.2);  color: var(--yellow); border: 1px solid rgba(234,179,8,.3); }
  .btn-purple { background: rgba(168,85,247,.2); color: var(--purple); border: 1px solid rgba(168,85,247,.3); }
  .btn-full   { grid-column: 1/-1; }

  /* Divider */
  .divider { border: none; border-top: 1px solid var(--border); margin: 14px 0; }

  /* Note */
  .note { font-size: 0.75rem; color: var(--text2); margin-top: 8px; text-align: center; min-height: 14px; }

  /* Jadwal */
  .jadwal-row { display: flex; align-items: center; gap: 10px; margin-bottom: 10px; }
  .jadwal-row label { font-size: 0.82rem; color: var(--text2); width: 50px; }
  .jadwal-row input[type=time] {
    flex: 1; padding: 10px 12px;
    background: var(--surface2); border: 1px solid var(--border);
    border-radius: 10px; color: var(--text); font-size: 0.9rem;
    outline: none; transition: border-color .2s;
  }
  .jadwal-row input[type=time]:focus { border-color: var(--purple); }
  .jadwal-row input[type=time]::-webkit-calendar-picker-indicator { filter: invert(1) opacity(.5); }
  .btn-save {
    width: 100%; padding: 12px; margin-top: 4px;
    background: linear-gradient(135deg, rgba(168,85,247,.3), rgba(59,130,246,.3));
    color: var(--text); border: 1px solid rgba(168,85,247,.4);
    border-radius: 10px; font-weight: 600; font-size: 0.9rem; cursor: pointer;
    transition: all .15s; letter-spacing: .3px;
  }
  .btn-save:hover { filter: brightness(1.2); transform: translateY(-1px); }

  /* Toast */
  .toast {
    position: fixed; bottom: 24px; left: 50%; transform: translateX(-50%) translateY(80px);
    background: var(--surface); border: 1px solid var(--border); color: var(--text);
    padding: 10px 20px; border-radius: 20px; font-size: 0.85rem; font-weight: 500;
    transition: transform .3s ease; z-index: 999; white-space: nowrap;
    box-shadow: 0 4px 20px rgba(0,0,0,.4);
  }
  .toast.show { transform: translateX(-50%) translateY(0); }

  /* RTOS badge */
  .rtos-info { text-align: center; margin-top: 16px; }
  .rtos-info span { font-size: 0.7rem; color: var(--text2); background: var(--surface2); padding: 4px 12px; border-radius: 20px; border: 1px solid var(--border); }
</style>
</head>
<body>
<div class="container">

  <div class="header">
    <h1>Smart Home ESP32</h1>
    <div class="clock" id="jam">--:--:--<span class="dot-pulse"></span></div>
  </div>

  <!-- Status -->
  <div class="card">
    <div class="card-title">Status Sistem</div>
    <div class="status-grid">
      <div class="status-item">
        <span class="status-label">Sensor Hujan</span>
        <span class="status-val" id="sHujan">-</span>
      </div>
      <div class="status-item">
        <span class="status-label">Posisi Atap</span>
        <span class="status-val" id="sServo">-</span>
      </div>
      <div class="status-item">
        <span class="status-label">Lampu</span>
        <span class="status-val" id="sLampu">-</span>
      </div>
      <div class="status-item">
        <span class="status-label">Mode Atap</span>
        <span class="badge" id="badgeAtap">-</span>
      </div>
      <div class="status-item">
        <span class="status-label">Mode Lampu</span>
        <span class="badge" id="badgeLampu">-</span>
      </div>
    </div>
  </div>

  <!-- Kontrol Atap -->
  <div class="card">
    <div class="card-title">Kontrol Atap</div>
    <div class="btn-group">
      <button class="btn btn-green"  onclick="kirim('/mode?v=auto')">⚙ Otomatis</button>
      <button class="btn btn-orange" onclick="kirim('/mode?v=manual')">✋ Manual</button>
    </div>
    <hr class="divider">
    <div class="btn-group">
      <button class="btn btn-blue" id="btnBuka"  onclick="kirim('/servo?v=buka')">▲ Buka</button>
      <button class="btn btn-gray" id="btnTutup" onclick="kirim('/servo?v=tutup')">▼ Tutup</button>
    </div>
    <p class="note" id="noteAtap"></p>
  </div>

  <!-- Kontrol Lampu -->
  <div class="card">
    <div class="card-title">Kontrol Lampu</div>
    <div class="btn-group">
      <button class="btn btn-blue"   onclick="kirim('/lampumode?v=manual')">✋ Manual</button>
      <button class="btn btn-purple" onclick="kirim('/lampumode?v=jadwal')">🕐 Jadwal</button>
    </div>
    <hr class="divider">
    <div class="btn-group">
      <button class="btn btn-yellow" id="btnOn"  onclick="kirim('/lampu?v=on')">☀ ON</button>
      <button class="btn btn-red"    id="btnOff" onclick="kirim('/lampu?v=off')">✕ OFF</button>
    </div>
    <p class="note" id="noteLampu"></p>
  </div>

  <!-- Jadwal -->
  <div class="card">
    <div class="card-title">Atur Jadwal Lampu</div>
    <div class="jadwal-row">
      <label>Nyala</label>
      <input type="time" id="inNyala">
    </div>
    <div class="jadwal-row">
      <label>Mati</label>
      <input type="time" id="inMati">
    </div>
    <button class="btn-save" onclick="simpanJadwal()">Simpan Jadwal</button>
    <p class="note">Jadwal aktif hanya saat mode Jadwal dipilih</p>
  </div>

  <div class="rtos-info">
    <span>⚡ FreeRTOS — 3 Task Aktif (Sensor · Jadwal · Web)</span>
  </div>
</div>

<div class="toast" id="toast"></div>

<script>
// ── Jam browser ──────────────────────────────────────
function updateJam() {
  var now = new Date();
  var h = String(now.getHours()).padStart(2,'0');
  var m = String(now.getMinutes()).padStart(2,'0');
  var s = String(now.getSeconds()).padStart(2,'0');
  document.getElementById('jam').innerHTML = h+':'+m+':'+s+'<span class="dot-pulse"></span>';
}
setInterval(updateJam, 1000);
updateJam();

// ── Sync jam ke ESP32 ────────────────────────────────
function syncJam() {
  var now = new Date();
  fetch('/syncjam?h='+now.getHours()+'&m='+now.getMinutes()).catch(function(){});
}
syncJam();
setInterval(syncJam, 60000);

// ── Toast notif ──────────────────────────────────────
function showToast(msg) {
  var t = document.getElementById('toast');
  t.textContent = msg;
  t.classList.add('show');
  setTimeout(function(){ t.classList.remove('show'); }, 2500);
}

// ── Kirim perintah ───────────────────────────────────
function kirim(url) {
  fetch(url)
    .then(function(r){ if(r.ok) updateStatus(); })
    .catch(function(){ showToast('Koneksi gagal'); });
}

// ── Flag: apakah user sedang edit jadwal ─────────────
// FIX: input tidak di-overwrite saat user lagi ngedit
var sedangEdit = false;
document.getElementById('inNyala').addEventListener('focus', function(){ sedangEdit = true; });
document.getElementById('inMati').addEventListener('focus',  function(){ sedangEdit = true; });
document.getElementById('inNyala').addEventListener('blur',  function(){ setTimeout(function(){ sedangEdit = false; }, 500); });
document.getElementById('inMati').addEventListener('blur',   function(){ setTimeout(function(){ sedangEdit = false; }, 500); });

// ── Simpan jadwal ────────────────────────────────────
function simpanJadwal() {
  var nyala = document.getElementById('inNyala').value;
  var mati  = document.getElementById('inMati').value;
  if (!nyala || !mati) { showToast('Isi jam nyala & mati dulu!'); return; }
  var now = new Date();
  fetch('/syncjam?h='+now.getHours()+'&m='+now.getMinutes())
    .then(function(){ return fetch('/jadwal?nyala='+nyala+'&mati='+mati); })
    .then(function(r){ if(r.ok){ showToast('✓ Jadwal tersimpan!'); sedangEdit = false; updateStatus(); } })
    .catch(function(){ showToast('Gagal simpan jadwal'); });
}

// ── Update status (polling tiap 4 detik) ─────────────
// Polling 4 detik lebih ringan dari 3 detik
function updateStatus() {
  fetch('/status')
    .then(function(r){ return r.json(); })
    .then(function(d){
      // Sensor hujan
      var sHujan = document.getElementById('sHujan');
      if (d.hujan) {
        sHujan.innerHTML = '<span class="dot dot-blue"></span>Hujan';
        sHujan.style.color = '#3b82f6';
      } else {
        sHujan.innerHTML = '<span class="dot dot-green"></span>Cerah';
        sHujan.style.color = '#22c55e';
      }

      // Servo
      var sServo = document.getElementById('sServo');
      if (d.servo) {
        sServo.innerHTML = '<span class="dot dot-green"></span>Terbuka';
        sServo.style.color = '#22c55e';
      } else {
        sServo.innerHTML = '<span class="dot dot-gray"></span>Tertutup';
        sServo.style.color = '#94a3b8';
      }

      // Lampu
      var sLampu = document.getElementById('sLampu');
      if (d.lampu) {
        sLampu.innerHTML = '<span class="dot dot-green"></span>Menyala';
        sLampu.style.color = '#eab308';
      } else {
        sLampu.innerHTML = '<span class="dot dot-gray"></span>Mati';
        sLampu.style.color = '#94a3b8';
      }

      // Badge mode atap
      var ba = document.getElementById('badgeAtap');
      ba.textContent = d.modeAtap ? 'Otomatis' : 'Manual';
      ba.className   = 'badge ' + (d.modeAtap ? 'badge-green' : 'badge-orange');

      // Badge mode lampu
      var bl = document.getElementById('badgeLampu');
      bl.textContent = d.modeLampu ? 'Jadwal' : 'Manual';
      bl.className   = 'badge ' + (d.modeLampu ? 'badge-purple' : 'badge-blue');

      // Disable tombol manual atap saat otomatis
      document.getElementById('btnBuka').disabled  = d.modeAtap;
      document.getElementById('btnTutup').disabled = d.modeAtap;
      document.getElementById('noteAtap').textContent = d.modeAtap ? 'Mode Otomatis aktif — ganti ke Manual untuk kontrol' : '';

      // Disable tombol lampu saat jadwal
      document.getElementById('btnOn').disabled  = d.modeLampu;
      document.getElementById('btnOff').disabled = d.modeLampu;
      document.getElementById('noteLampu').textContent = d.modeLampu ? 'Mode Jadwal aktif — lampu dikontrol otomatis' : '';

      // FIX: update input jadwal HANYA saat user tidak sedang ngedit
      if (!sedangEdit) {
        document.getElementById('inNyala').value = d.jadwalNyala;
        document.getElementById('inMati').value  = d.jadwalMati;
      }
    })
    .catch(function(){
      document.getElementById('sHujan').textContent = 'Koneksi terputus...';
    });
}

updateStatus();
setInterval(updateStatus, 4000);
</script>
</body>
</html>)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", HTML_PAGE);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  pinMode(RAIN_SENSOR, INPUT);
  pinMode(LAMPU_PIN, OUTPUT);
  digitalWrite(LAMPU_PIN, LOW);

  s1.attach(SERVO_PIN);
  s1.write(0);

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

  // ===== 3 TASK FREERTOS =====
  xTaskCreatePinnedToCore(taskSensor, "Sensor", 2048, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(taskJadwal, "Jadwal", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(taskWeb,    "Web",    8192, NULL, 1, NULL, 0);
}

// loop ditidurkan — semua logic di task
void loop() {
  vTaskDelay(portMAX_DELAY);
}