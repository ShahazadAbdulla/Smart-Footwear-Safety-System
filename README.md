# Smart Footwear Safety System 🛡️👟

An advanced, IoT-based wearable safety device integrated into footwear, designed to provide immediate, failsafe assistance in emergency situations. 

![WhatsApp Image 2026-03-24 at 12 08 52](https://github.com/user-attachments/assets/7637511b-55b4-4ffb-b281-d3e234ef5846)

https://youtu.be/qBMRTrj5U98

https://github.com/user-attachments/assets/60a631ea-56f0-48fb-ad00-363e10b8a0da


https://github.com/user-attachments/assets/11c4666f-ae9d-4d68-956d-c3f41e0974a6


## Core Architecture
This system utilizes **Dual-Core Processing (FreeRTOS)** on the ESP32 to ensure critical safety sensors are never blocked by network latency.
* **Core 0 (Hardware Watchdog):** Dedicated entirely to monitoring the Piezoelectric impact sensor. It runs continuously, immune to Wi-Fi drops or GPS delays.
* **Core 1 (Comms & Logic):** Handles Telegram API communication, GPS parsing, and Camera triggering.

## Failsafe Features
* **Smart Location Fallback:** If the GPS module cannot acquire a satellite lock (e.g., indoors), the system automatically falls back to an IP-based Geolocation network estimate.
* **Dual-Stage Escalation:** Alerts the Guardian first. If the Guardian does not reply "SAFE" within 45 seconds, it automatically escalates the alert and GPS coordinates to Police/Authorities.
* **Continuous Alarm:** Loud buzzer (Pin 23) activates immediately to deter attackers.
* **Video Evidence:** Wirelessly triggers a secondary ESP32-CAM module to record 30 seconds of high-quality video directly to a local SD card (bypassing Wi-Fi lag).
* **Self-Sustaining:** Powered by Piezoelectric Energy Harvesting crystals in the sole, charging a local Li-ion battery via a BMS.

## Hardware Pinout (ESP32 Main)
* **Piezo Impact Sensor:** GPIO 32
* **Impact Indicator LED:** GPIO 25
* **Panic Button:** GPIO 22 (Internal Pullup)
* **Active Buzzer:** GPIO 23
* **GPS RX:** GPIO 16 (ESP32 RX2)
* **GPS TX:** GPIO 17 (ESP32 TX2)
* **Camera Trigger:** UDP Wireless Broadcast (Port 4210)

## Operating Protocol
1. Power on the system. The ESP32 will acquire a Wi-Fi backup location immediately.
2. The GPS will search in the background. Core 0 arms the Piezo sensor.
3. **Impact/Button Press:** Triggers the SOS sequence.
4. **Guardian Alert** sent via Telegram with the best available location (GPS or Wi-Fi).
5. **Camera** begins recording evidence.
6. **Buzzer** sounds continuously.
7. Reply "SAFE" on Telegram to cancel, or wait 45s for Automatic Police Alert.
