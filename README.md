# Sistem-Mikrokontroller-Kelompok 2

<h2>JUDUL PROYEK</h2>
<ul>
    Implementasi Smart Lighting Menggunakan ESP32 dan MQTT untuk Monitoring dan Kontrol Realtime
</ul>

<h2>PENJELASAN PROYEK</h2>
<ul>
    Pengguna dapat mengakses web dashboard untuk melihat status lampu, menyalakan atau mematikan lampu dari jarak jauh, serta mengatur jadwal otomatis sesuai kebutuhan. Komunikasi data antara perangkat dan dashboard menggunakan protokol MQTT sehingga proses pengiriman dan penerimaan data dapat berlangsung secara cepat dan ringan.

Selain fitur kontrol, sistem juga menyimpan histori penggunaan lampu yang dapat digunakan untuk menganalisis pola pemakaian. Dengan adanya fitur otomatisasi dan monitoring realtime, pengguna dapat mengurangi pemborosan energi akibat lampu yang lupa dimatikan serta meningkatkan efisiensi penggunaan listrik di ruang tamu.</ul>

<h2>CARA KERJA</h2>
<ul>
    <li><strong>1. ESP32 terhubung ke jaringan WiFi.
2. Lampu dihubungkan ke relay yang dikendalikan oleh ESP32.
3. Dashboard web mengirim perintah ON/OFF melalui MQTT.
4. ESP32 menerima perintah dan mengaktifkan relay.
5. Status lampu dikirim kembali ke server.
6. Dashboard menampilkan status lampu secara realtime.
7. Sistem dapat menjalankan jadwal otomatis yang telah ditentukan pengguna.</strong></li>
</ul>

<h2>KOMPONEN PROYEK</h2>
<ul>
    <li><strong>Hardware
ESP32
Relay Module 1 Channel
Lampu LED / Bohlam
Breadboard
Kabel Jumper
Adaptor USB
        
Software
Arduino IDE
MQTT Broker (Mosquitto/HiveMQ)
Web Dashboard
Database MySQL
XAMPP/Laragon</strong></li>
</ul>

<h3>Disusun Oleh:</h3>
<ul>
    <li><strong>Muhammad Nizham Hibatullah</strong> (23552011241)</li>
    <li><strong>Sheva Nadhif Gazzauhar</strong> (23552011018)</li>
    <li><strong>Annisa Nur Fitriani</strong> (23552011192)</li>
</ul>
