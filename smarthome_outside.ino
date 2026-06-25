#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

// ===== KONFIGURASI =====
const char* ssid     = "花の名";
const char* password = "higanbana";

#define RAIN_SENSOR  14
#define SERVO_PIN    27
#define LAMPU_PIN    26   // tambah relay/LED di pin 26

// ===== OBJEK =====
Servo s1;
WebServer server(80);

// ===== STATE =====
bool modeOtomatis = true;   // default: otomatis
bool servoTerbuka = false;  // false = tutup (0°), true = buka (120°)
bool lampuNyala   = false;

// ===== FUNGSI GERAK SERVO =====
void gerakServo(bool buka) {
  servoTerbuka = buka;
  s1.write(buka ? 120 : 0);
}

// ===== HALAMAN WEB (HTML) =====
String buatHalaman() {
  int rainVal = digitalRead(RAIN_SENSOR);
  String statusHujan  = (rainVal == LOW) ? "🌧️ Hujan" : "☀️ Tidak Hujan";
  String statusServo  = servoTerbuka ? "Terbuka (120°)" : "Tertutup (0°)";
  String statusLampu  = lampuNyala ? "ON 💡" : "OFF";
  String statusMode   = modeOtomatis ? "Otomatis" : "Manual";
  String warnaBadge   = modeOtomatis ? "#4CAF50" : "#FF9800";

  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Kontrol Atap & Lampu</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: 'Segoe UI', sans-serif; background: #f0f4f8; color: #333; }
    .container { max-width: 480px; margin: 30px auto; padding: 16px; }
    h1 { text-align: center; margin-bottom: 20px; color: #1a237e; font-size: 1.4rem; }

    .card {
      background: white; border-radius: 12px;
      padding: 16px 20px; margin-bottom: 16px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.08);
    }
    .card h2 { font-size: 0.85rem; color: #888; margin-bottom: 10px; text-transform: uppercase; letter-spacing: 1px; }

    .status-row { display: flex; justify-content: space-between; align-items: center; margin: 6px 0; }
    .badge {
      padding: 4px 12px; border-radius: 20px; font-size: 0.85rem; font-weight: 600;
      background: )rawliteral" + warnaBadge + R"rawliteral(; color: white;
    }

    .btn-row { display: flex; gap: 10px; margin-top: 12px; }
    .btn {
      flex: 1; padding: 12px; border: none; border-radius: 8px;
      font-size: 0.95rem; font-weight: 600; cursor: pointer; transition: opacity 0.2s;
    }
    .btn:hover { opacity: 0.85; }
    .btn-green  { background: #4CAF50; color: white; }
    .btn-blue   { background: #2196F3; color: white; }
    .btn-orange { background: #FF9800; color: white; }
    .btn-red    { background: #f44336; color: white; }
    .btn-gray   { background: #9E9E9E; color: white; }
    .btn-yellow { background: #FFC107; color: #333; }

    .auto-note { font-size: 0.8rem; color: #999; margin-top: 8px; text-align: center; }
    .refresh { text-align: center; margin-top: 20px; font-size: 0.8rem; color: #aaa; }
  </style>
  <meta http-equiv="refresh" content="3">
</head>
<body>
<div class="container">
  <h1>🏠 Kontrol Atap & Lampu</h1>

  <!-- STATUS -->
  <div class="card">
    <h2>Status Saat Ini</h2>
    <div class="status-row"><span>🌧️ Sensor Hujan</span><strong>)rawliteral" + statusHujan + R"rawliteral(</strong></div>
    <div class="status-row"><span>🔧 Atap</span><strong>)rawliteral" + statusServo + R"rawliteral(</strong></div>
    <div class="status-row"><span>💡 Lampu</span><strong>)rawliteral" + statusLampu + R"rawliteral(</strong></div>
    <div class="status-row"><span>⚙️ Mode</span><span class="badge">)rawliteral" + statusMode + R"rawliteral(</span></div>
  </div>

  <!-- MODE -->
  <div class="card">
    <h2>Mode Kontrol Atap</h2>
    <div class="btn-row">
      <button class="btn btn-green" onclick="fetch('/mode?v=auto')  .then(()=>location.reload())">🤖 Otomatis</button>
      <button class="btn btn-orange" onclick="fetch('/mode?v=manual').then(()=>location.reload())">🖐️ Manual</button>
    </div>
  </div>

  <!-- KONTROL SERVO (hanya aktif di mode manual) -->
  <div class="card">
    <h2>Kontrol Atap (Manual)</h2>
    <div class="btn-row">
      <button class="btn btn-blue" onclick="fetch('/servo?v=buka') .then(()=>location.reload())" )rawliteral" + (modeOtomatis ? "disabled style='opacity:0.4'" : "") + R"rawliteral(>⬆️ Buka</button>
      <button class="btn btn-gray" onclick="fetch('/servo?v=tutup').then(()=>location.reload())" )rawliteral" + (modeOtomatis ? "disabled style='opacity:0.4'" : "") + R"rawliteral(>⬇️ Tutup</button>
    </div>
    )rawliteral" + (modeOtomatis ? "<p class='auto-note'>Ganti ke mode Manual untuk kontrol manual</p>" : "") + R"rawliteral(
  </div>

  <!-- KONTROL LAMPU -->
  <div class="card">
    <h2>Kontrol Lampu</h2>
    <div class="btn-row">
      <button class="btn btn-yellow" onclick="fetch('/lampu?v=on') .then(()=>location.reload())">💡 ON</button>
      <button class="btn btn-red"    onclick="fetch('/lampu?v=off').then(()=>location.reload())">🌑 OFF</button>
    </div>
  </div>

  <p class="refresh">🔄 Auto-refresh tiap 3 detik</p>
</div>
</body>
</html>
)rawliteral";

  return html;
}

// ===== ROUTE HANDLER =====
void handleRoot() {
  server.send(200, "text/html", buatHalaman());
}

void handleMode() {
  if (server.hasArg("v")) {
    modeOtomatis = (server.arg("v") == "auto");
  }
  server.send(200, "text/plain", "OK");
}

void handleServo() {
  if (server.hasArg("v") && !modeOtomatis) {
    gerakServo(server.arg("v") == "buka");
  }
  server.send(200, "text/plain", "OK");
}

void handleLampu() {
  if (server.hasArg("v")) {
    lampuNyala = (server.arg("v") == "on");
    digitalWrite(LAMPU_PIN, lampuNyala ? HIGH : LOW);
  }
  server.send(200, "text/plain", "OK");
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  pinMode(RAIN_SENSOR, INPUT);
  pinMode(LAMPU_PIN, OUTPUT);
  digitalWrite(LAMPU_PIN, LOW);

  s1.attach(SERVO_PIN);
  s1.write(0);

  // Konek WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  // Daftarkan route
  server.on("/",      handleRoot);
  server.on("/mode",  handleMode);
  server.on("/servo", handleServo);
  server.on("/lampu", handleLampu);
  server.begin();
}

// ===== LOOP =====
void loop() {
  server.handleClient();

  // Logika otomatis (hanya jalan kalau mode otomatis)
  if (modeOtomatis) {
    int val = digitalRead(RAIN_SENSOR);
    if (val == LOW) {
      Serial.println("Hujan → Tutup");
      gerakServo(false);  // tutup saat hujan
    } else {
      Serial.println("Tidak Hujan → Buka");
      gerakServo(true);   // buka saat cerah
    }
  }

  delay(100);
}