# 🚦 ESP32 Traffic Light AI (FreeRTOS + MQTT)

Project ini merupakan implementasi sistem **Traffic Light berbasis ESP32** yang berjalan
secara **real-time**, **offline-capable**, dan terhubung ke jaringan menggunakan **MQTT**.
Sistem dirancang sesuai konsep **Embedded System modern** dengan pemanfaatan FreeRTOS.

---

## ✨ Fitur Utama

### 🧠 FreeRTOS (Manajemen Proses)
- Sistem dibagi ke dalam beberapa **task FreeRTOS**:
  - `trafficTask` → logika traffic light & AI sederhana
  - `wifiTask` → koneksi WiFi (non-blocking)
  - `mqttTask` → komunikasi MQTT (jika online)
- Setiap task berjalan **independen**, sehingga kegagalan WiFi **tidak menghentikan sistem**.

---

### 💡 PWM (LED Control)
- Output lampu **RED, YELLOW, GREEN** dikontrol menggunakan **PWM (LEDC ESP32)**.
- PWM memungkinkan:
  - Efek **fade** (lampu kuning)
  - Intensitas lampu yang stabil
- PWM tetap aktif meskipun sistem **offline**.

---c:\Users\jaman\Downloads\carbon.png

### 💾 EEPROM / Preferences
- Nilai **AI Score** dan konfigurasi sistem disimpan menggunakan `Preferences`.
- Data **tidak hilang** saat:
  - Restart
  - Power off
- Sistem dapat melanjutkan kondisi terakhir secara otomatis.

---

### 🌐 MQTT (Komunikasi IoT)
- ESP32 mengirim status sistem dalam format **JSON** ke broker MQTT:
  - Status lampu
  - Mode sistem
  - AI score
  - Durasi lampu hijau
- Mendukung **control topic** untuk kontrol jarak jauh.
- Jika MQTT/WiFi terputus:
  - Sistem tetap berjalan (offline-first design)
  - Reconnect dilakukan saat koneksi tersedia kembali.

---

## 📦 Teknologi yang Digunakan
- ESP32
- FreeRTOS
- PWM (LEDC)
- Preferences (EEPROM)
- MQTT (HiveMQ Cloud)

---

## 📌 Catatan
Project ini dibuat untuk keperluan **pembelajaran Embedded System & IoT**, dengan fokus
pada manajemen proses, memori, dan komunikasi jaringan secara terstruktur.

---
