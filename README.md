# Smart Footwear Safety System 🛡️👟

A wearable IoT safety device integrated into footwear designed to provide immediate assistance in emergency situations. It features GPS tracking, video evidence recording, and a dual-stage escalation alert system.

## Features
* **Panic Button Trigger:** One-press activation.
* **Continuous Alarm:** Loud buzzer (Pin 23) activates immediately to deter attackers.
* **Live GPS Tracking:** Sends real-time Google Maps location to a Guardian.
* **Evidence Recording:** Wirelessly triggers an ESP32-CAM to record 30s of video to SD card.
* **Escalation Logic:** If the Guardian does not reply "SAFE" within 45 seconds, a second alert with GPS is sent to the Authorities (Police).
* **Self-Sustaining:** Powered by Piezoelectric Energy Harvesting crystals in the sole.

## Hardware Pinout (ESP32 Main)
* **Button:** GPIO 22 (Internal Pullup)
* **Buzzer:** GPIO 23
* **GPS RX:** GPIO 16
* **GPS TX:** GPIO 17
* **Camera Trigger:** UDP Wireless (Port 4210)

## How to Use
1.  Power on the system.
2.  Wait for the buzzer to stop "pip" beeping (indicates GPS Lock).
3.  Press the Button (Pin 22).
4.  **Guardian Alert** is sent via Telegram.
5.  **Camera** starts recording.
6.  **Buzzer** sounds continuously.
7.  Reply "SAFE" to cancel, or wait 45s for Police Alert.
