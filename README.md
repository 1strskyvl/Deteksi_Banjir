#  Sistem Pemantauan & Deteksi Dini Banjir Berbasis ESP32

> **Gambaran Proyek (Overview):**  
> Repositori ini berisi program sistem pendeteksi dini banjir menggunakan mikrokontroler **ESP32** dan framework **ESP-IDF (C)**. Program ini dirancang untuk membaca ketinggian air secara *real-time* berbasis sensor ultrasonik.  
> 
> Kode yang ada di repositori ini **dapat digunakan secara langsung untuk alat fisik di dunia nyata maupun dijalankan di platform simulasi seperti Wokwi**.

---

##  Konsep Kerja Sistem
- **Semakin Jauh Jarak yang Terbaca:** Permukaan air rendah/dangkal $\rightarrow$ **Status AMAN**
- **Semakin Dekat Jarak yang Terbaca:** Permukaan air meluap mendekati sensor $\rightarrow$ **Status BAHAYA**

##  Komponen yang Digunakan

1. **ESP32 Development Board**
2. **HC-SR04** (Sensor Ultrasonik)
3. **LCD 16x2 dengan Modul I2C** (PCF8574)
4. **3x LED** (Hijau, Kuning, Merah)
5. **Buzzer Active**
6. **Resistor** (220Ω untuk LED)

---

##  Skema Wiring & Pinout

| Komponen | Pin Modul | Pin ESP32 | Keterangan |
| :--- | :--- | :--- | :--- |
| **HC-SR04** | TRIG | **GPIO 5** | Trigger Sinyal |
| | ECHO | **GPIO 18** | Echo Sinyal Input |
| **LCD 16x2 I2C** | SDA | **GPIO 21** | Data Bus I2C |
| | SCL | **GPIO 22** | Clock Bus I2C |
| **LED Indikator** | LED Hijau | **GPIO 12** | Status Aman |
| | LED Kuning | **GPIO 14** | Status Siaga |
| | LED Merah | **GPIO 27** | Status Bahaya |
| **Buzzer** | VCC / Signal | **GPIO 26** | Alarm Peringatan Suara |

---

##  Batas Indikator Ketinggian Air (Threshold)

- 🟢 **AMAN (`> 150 cm`)**: Air berada di batas normal. LED Hijau menyala.
- 🟡 **SIAGA (`50 cm - 150 cm`)**: Air mulai naik mendekati pemukiman/jembatan. LED Kuning menyala.
- 🔴 **BAHAYA (`< 50 cm`)**: Air berada di level kritis/hampir meluap. LED Merah menyala dan Buzzer berbunyi secara intermiten (*beep-beep*).

---

##  Cara Menjalankan Program
### Jalankan via Simulasi (Wokwi)
1. Buka [Wokwi.com](https://wokwi.com/) dan buat proyek baru **ESP32 (ESP-IDF)**.
2. Salin seluruh isi file `main.c` ke tab `main.c` di Wokwi.
3. Salin isi file `diagram.json` ke tab `diagram.json` di Wokwi untuk membuat komponen & rangkaian kabelnya.
4. Klik tombol **Start Simulation**.
