# Sistem-Mikrokontroller-Kelompok 2

<h2>JUDUL PROYEK</h2>
<ul>
    Implementasi Smart Lighting Menggunakan ESP32 dan MQTT untuk Monitoring dan Kontrol Realtime
</ul>

<h2>PENJELASAN PROYEK</h2>
<ul>
    Pengguna dapat mengakses web dashboard untuk melihat status lampu, menyalakan atau mematikan lampu dari jarak jauh, serta mengatur jadwal otomatis sesuai kebutuhan. Komunikasi data antara perangkat dan dashboard menggunakan protokol MQTT sehingga proses pengiriman dan penerimaan data dapat berlangsung secara cepat dan ringan. 
    <p>Selain fitur kontrol, sistem juga menyimpan histori penggunaan lampu yang dapat digunakan untuk menganalisis pola pemakaian. Dengan adanya fitur otomatisasi dan monitoring realtime, pengguna dapat mengurangi pemborosan energi akibat lampu yang lupa dimatikan serta meningkatkan efisiensi penggunaan listrik di ruang tamu.</ul>

<h2>CARA KERJA</h2>
<ul>
<p>1. ESP32 terhubung ke jaringan WiFi.
<p>2. Lampu dihubungkan ke relay yang dikendalikan oleh ESP32.
<p>3. Dashboard web mengirim perintah ON/OFF melalui MQTT.
<p>4. ESP32 menerima perintah dan mengaktifkan relay.
<p>5. Status lampu dikirim kembali ke server.
<p>6. Dashboard menampilkan status lampu secara realtime.
<p>7. Sistem dapat menjalankan jadwal otomatis yang telah ditentukan pengguna.
</ul>

<h2>KOMPONEN PROYEK</h2>
<ul>
<p><strong>HARDWARE:</strong>
<p>1. ESP32
<p>2. Relay Module 1 Channel
<p>3. Lampu LED / Bohlam
<p>4. Breadboard
<p>5. Kabel Jumper
<p>6. Adaptor USB
<p><strong>SOFTWARE:</strong>
<p>1. Arduino IDE
<p>2. MQTT Broker (Mosquitto/HiveMQ)
<p>3. Web Dashboard
<p>4. Database MySQL
<p>5. XAMPP/Laragon
</ul>

<h3>Disusun Oleh:</h3>
<ul>
    <li><strong>Muhammad Nizham Hibatullah</strong> (23552011241)</li>
    <li><strong>Sheva Nadhif Gazzauhar</strong> (23552011018)</li>
    <li><strong>Annisa Nur Fitriani</strong> (23552011192)</li>
</ul>
