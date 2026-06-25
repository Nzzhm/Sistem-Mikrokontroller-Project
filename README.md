# 🌧️ Smart Jemuran Dan Lampu Teras Otomatis Berbasis ESP32 dan HTTP

> **Sistem Mikrokontroller – Kelompok 2**

---

## 📌 Judul Proyek
**Implementasi Smart Home Berbasis ESP32 dan HTTP dengan Sistem Jemuran Dan Lampu Teras Otomatis**

---

## 📖 Deskripsi Proyek

Proyek ini bertujuan untuk mengatasi masalah ketidakpastian cuaca, khususnya hujan mendadak, yang sering mengganggu proses penjemuran pakaian. Dengan memanfaatkan teknologi **Internet of Things (IoT)**, sistem ini secara otomatis mendeteksi adanya tetesan air dan menggerakkan jemuran ke area terlindung tanpa perlu campur tangan pengguna.

Sistem dibangun menggunakan mikrokontroler **ESP32** yang terhubung ke **sensor tetesan air** dan **motor servo** sebagai penggerak. Semua data dan kendali sistem dikomunikasikan melalui protokol **HTTP**, sehingga pengguna dapat memantau dan mengontrol jemuran dari jarak jauh melalui dashboard web.

Dengan adanya sistem ini, pengguna tidak perlu khawatir lagi saat meninggalkan rumah dalam kondisi cuaca mendung, karena jemuran akan otomatis tertarik saat hujan mulai turun.

---

## ⚙️ Cara Kerja Sistem

1. ESP32 terhubung ke jaringan WiFi.
2. pada website bisa memilih otomatis atau manual untuk jewmuran otomatis dan lampu teras otomatis.
3. jika jemuran otomatis, saat sensor rain terkena air maka servo/jemuran otomatis akan menutup dan jika sensor rain tidak terkena air maka servo/jemuran otomatis terbuka.
4. jika jemuran manual, kita bisa kapan saja membuka dan menutup servo/jemuran manual di web.
5. jika lampu teras otomatis, kita dapat setting waktu sesuai kemauan kita, misal hidup jam 06.00 dan mati di jam 12.00.
6. jika lampu teras manual, kita dapat menyalakan dan mematikan lampu secara manual di web.

---

## 🧩 Komponen Proyek

### 🔧 Hardware
1. ESP32
2. Rain Drop Module
3. Servo SG90
4. Kabel Jumper
5. Adaptor USB
6. LED

### 💻 Software
1. Arduino IDE
2. HTTP
3. Web Dashboard

---

## 📊 Fitur Utama

- ✅ Deteksi otomatis tetesan air / hujan
- ✅ Penggerak jemuran menggunakan motor servo
- ✅ Monitoring real-time melalui dashboard web
- ✅ Kontrol manual dari jarak jauh (via HTTP)

---

## 👥 Anggota Kelompok
<ul>
    <li><strong>Muhammad Nizham Hibatullah</strong> (23552011241)</li>
    <li><strong>Sheva Nadhif Gazzauhar</strong> (23552011018)</li>
    <li><strong>Annisa Nur Fitriani</strong> (23552011192)</li>
</ul>
