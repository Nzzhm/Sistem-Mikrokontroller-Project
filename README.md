# 🌧️ Smart Jemuran Otomatis Berbasis ESP32 dan MQTT

> **Sistem Mikrokontroller – Kelompok 2**

---

## 📌 Judul Proyek
**Implementasi Smart Home Berbasis ESP32 dan MQTT dengan Sistem Jemuran Otomatis**

---

## 📖 Deskripsi Proyek

Proyek ini bertujuan untuk mengatasi masalah ketidakpastian cuaca, khususnya hujan mendadak, yang sering mengganggu proses penjemuran pakaian. Dengan memanfaatkan teknologi **Internet of Things (IoT)**, sistem ini secara otomatis mendeteksi adanya tetesan air dan menggerakkan jemuran ke area terlindung tanpa perlu campur tangan pengguna.

Sistem dibangun menggunakan mikrokontroler **ESP32** yang terhubung ke **sensor tetesan air** dan **motor servo** sebagai penggerak. Semua data dan kendali sistem dikomunikasikan melalui protokol **MQTT**, sehingga pengguna dapat memantau dan mengontrol jemuran dari jarak jauh melalui dashboard web.

Dengan adanya sistem ini, pengguna tidak perlu khawatir lagi saat meninggalkan rumah dalam kondisi cuaca mendung, karena jemuran akan otomatis tertarik saat hujan mulai turun.

---

## ⚙️ Cara Kerja Sistem

1. ESP32 terhubung ke jaringan WiFi.
2. Lampu dihubungkan ke relay yang dikendalikan oleh ESP32.
3. Dashboard web mengirim perintah ON/OFF melalui MQTT.
4. ESP32 menerima perintah dan mengaktifkan relay.
5. Status lampu dikirim kembali ke server.
6. Dashboard menampilkan status lampu secara realtime.
7. Sistem dapat menjalankan jadwal otomatis yang telah ditentukan pengguna.

---

## 🧩 Komponen Proyek

### 🔧 Hardware
<p>1. ESP32
<p>2. Relay Module 1 Channel
<p>3. Lampu LED / Bohlam
<p>4. Breadboard
<p>5. Kabel Jumper
<p>6. Adaptor USB

### 💻 Software
<p>1. Arduino IDE
<p>2. MQTT Broker (Mosquitto/HiveMQ)
<p>3. Web Dashboard
<p>4. Database MySQL
<p>5. XAMPP/Laragon
</ul>

---

## 📊 Fitur Utama

- ✅ Deteksi otomatis tetesan air / hujan
- ✅ Penggerak jemuran menggunakan motor servo
- ✅ Monitoring real-time melalui dashboard web
- ✅ Kontrol manual dari jarak jauh (via MQTT)

---

## 👥 Anggota Kelompok
<ul>
    <li><strong>Muhammad Nizham Hibatullah</strong> (23552011241)</li>
    <li><strong>Sheva Nadhif Gazzauhar</strong> (23552011018)</li>
    <li><strong>Annisa Nur Fitriani</strong> (23552011192)</li>
</ul>
