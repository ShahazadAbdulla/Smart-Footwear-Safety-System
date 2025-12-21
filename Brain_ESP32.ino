#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <WiFiUdp.h>

// ================= CONFIGURATION =================
const char* ssid = "Wifi";
const char* password = "password";

// Telegram Credentials
#define BOT_TOKEN_GUARDIAN "8468870357:AAGmXJoE6G0GLNip7qBrpLFS1LsaP_m2JlA" 
#define BOT_TOKEN_AUTHORITY "8436575168:AAEA3kZ9IZOLkMscGnCgzbPzs3tYN6shkCg" 
#define CHAT_ID "5026129710"

// --- UPDATED PINS ---
#define BUTTON_PIN 22     // Button to Pin 22 and GND
#define BUZZER_PIN 23     // Buzzer (+) to Pin 23, (-) to GND
#define GPS_RX_PIN 16     
#define GPS_TX_PIN 17     

// --- TIMING ---
#define ESCALATION_TIME 45000 // 45 Seconds

// Wireless Signal (UDP)
WiFiUDP udp;
const int UDP_PORT = 4210;
IPAddress broadcastIP(255, 255, 255, 255); 

// Objects
WiFiClientSecure client;
UniversalTelegramBot botGuardian(BOT_TOKEN_GUARDIAN, client);
UniversalTelegramBot botAuthority(BOT_TOKEN_AUTHORITY, client);
TinyGPSPlus gps;
HardwareSerial GPS_Serial(2);

// Logic Variables
bool sosActive = false;
bool escalated = false;
unsigned long sosStartTime = 0;
unsigned long lastPollTime = 0;
unsigned long lastDebugPrint = 0;
unsigned long lastGpsBeep = 0; 

void setup() {
  Serial.begin(115200);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); 

  GPS_Serial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  Serial.print("\n[INIT] Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  client.setInsecure(); 
  client.setTimeout(2000); 
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[INIT] WiFi Connected!");

  Serial.print("[INIT] Cleaning old Telegram messages... ");
  int num = botGuardian.getUpdates(botGuardian.last_message_received + 1);
  while(num) {
    num = botGuardian.getUpdates(botGuardian.last_message_received + 1);
  }
  Serial.println("Clean.");
  
  udp.begin(UDP_PORT);
  
  Serial.println("------------------------------------------");
  Serial.println("SYSTEM READY.");
  Serial.println("Waiting for GPS (Will beep until locked)...");
  Serial.println("------------------------------------------");
}

void handleReplies(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(botGuardian.messages[i].chat_id);
    String text = botGuardian.messages[i].text;
    
    if (chat_id == CHAT_ID && text.equalsIgnoreCase("SAFE")) {
      sosActive = false;
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("[ALARM] Buzzer OFF.");
      botGuardian.sendMessage(CHAT_ID, "✅ SOS CANCELLED by Guardian.", "");
    }
  }
}

void loop() {
  while (GPS_Serial.available() > 0) gps.encode(GPS_Serial.read());

  // GPS Searching Beep
  if (!gps.location.isValid() && !sosActive) {
    if (millis() - lastGpsBeep > 3000) {
      lastGpsBeep = millis();
      digitalWrite(BUZZER_PIN, HIGH);
      delay(50);
      digitalWrite(BUZZER_PIN, LOW);
      Serial.print("."); 
    }
  }

  // Check Button
  if (digitalRead(BUTTON_PIN) == LOW && !sosActive) {
    delay(50); 
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("\n[EVENT] BUTTON PRESSED!");
      
      // Note: Change 'true' to 'gps.location.isValid()' for real deployment
      if (true) { 
        Serial.println("[ACTION] Starting SOS Sequence...");
        sosActive = true;
        escalated = false;
        sosStartTime = millis();

        digitalWrite(BUZZER_PIN, HIGH);
        Serial.println("[ALARM] Buzzer ON (Continuous)");

        Serial.print("[1/3] Sending Signal to Camera... ");
        udp.beginPacket(broadcastIP, UDP_PORT);
        udp.print("START_REC");
        udp.endPacket();
        Serial.println("SENT.");

        Serial.print("[2/3] Alerting Guardian... ");
        String msg = "🚨 SOS ALERT! 🚨\n";
        
        if(gps.location.isValid()){
           msg += "Lat: " + String(gps.location.lat(), 6);
           msg += "\nLng: " + String(gps.location.lng(), 6);
           msg += "\nMap: http://maps.google.com/maps?q=" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
        } else {
           msg += "Location: Acquiring GPS... (Beeping until fixed)";
        }
        msg += "\n\nReply 'SAFE' in 45s to stop Police escalation.";
        
        if(botGuardian.sendMessage(CHAT_ID, msg, "")){
          Serial.println("SENT.");
        } else {
          Serial.println("FAILED.");
        }
      }
    }
  }

  // SOS Active Logic
  if (sosActive) {
    if (millis() - lastPollTime > 3000) {
      int num = botGuardian.getUpdates(botGuardian.last_message_received + 1);
      if (num > 0) handleReplies(num);
      lastPollTime = millis();
    }

    if (sosActive && !escalated && (millis() - sosStartTime > ESCALATION_TIME)) {
      Serial.println("\n[TIMEOUT] 45s Passed. Escalating...");
      
      String msg = "⚠️ URGENT: POLICE ASSISTANCE REQUIRED ⚠️\n";
      msg += "Victim Unresponsive / Guardian Timeout.\n";
      
      if(gps.location.isValid()){
         msg += "CONFIRMED LOCATION:\n";
         msg += "Lat: " + String(gps.location.lat(), 6) + "\n";
         msg += "Lng: " + String(gps.location.lng(), 6) + "\n";
         msg += "Map: http://maps.google.com/maps?q=" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
      } else {
         msg += "Location: GPS Signal Lost/Searching...";
      }

      botAuthority.sendMessage(CHAT_ID, msg, "");
      
      escalated = true; 
      sosActive = false; 
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("[RESET] System Ready.");
    }
  }
}
