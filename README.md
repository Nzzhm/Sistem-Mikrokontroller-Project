# Sistem Mikrokontroller—Kelompok 2

## JUDUL PROYEK
Implementasi Smart Home Berbasis ESP32 dan MQTT dengan Sistem Jemuran Otomatis
<ul>
    Implementasi Smart Lighting Menggunakan ESP32 dan MQTT untuk Monitoring dan Kontrol Realtime
</ul>

## PENJELASAN PROYEK
<ul>
    Pengguna dapat mengakses web dashboard untuk melihat status lampu, menyalakan atau mematikan lampu dari jarak jauh, serta mengatur jadwal otomatis sesuai kebutuhan. Komunikasi data antara perangkat dan dashboard menggunakan protokol MQTT sehingga proses pengiriman dan penerimaan data dapat berlangsung secara cepat dan ringan. 
    <p>Selain fitur kontrol, sistem juga menyimpan histori penggunaan lampu yang dapat digunakan untuk menganalisis pola pemakaian. Dengan adanya fitur otomatisasi dan monitoring realtime, pengguna dapat mengurangi pemborosan energi akibat lampu yang lupa dimatikan serta meningkatkan efisiensi penggunaan listrik di ruang tamu.</ul>

## CARA KERJA

1. ESP32 terhubung ke jaringan WiFi.
2. Lampu dihubungkan ke relay yang dikendalikan oleh ESP32.
3. Dashboard web mengirim perintah ON/OFF melalui MQTT.
4. ESP32 menerima perintah dan mengaktifkan relay.
5. Status lampu dikirim kembali ke server.
6. Dashboard menampilkan status lampu secara realtime.
7. Sistem dapat menjalankan jadwal otomatis yang telah ditentukan pengguna.

## KOMPONEN PROYEK
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

## Disusun Oleh:
<ul>
    <li><strong>Muhammad Nizham Hibatullah</strong> (23552011241)</li>
    <li><strong>Sheva Nadhif Gazzauhar</strong> (23552011018)</li>
    <li><strong>Annisa Nur Fitriani</strong> (23552011192)</li>
</ul>
