#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>

// ================= CONFIGURATION =================
const char* ssid = "Wifi";
const char* password = "password";

// Telegram Credentials
#define BOT_TOKEN_GUARDIAN "8468870357:AAGmXJoE6G0GLNip7qBrpLFS1LsaP_m2JlA" 
#define BOT_TOKEN_AUTHORITY "8436575168:AAEA3kZ9IZOLkMscGnCgzbPzs3tYN6shkCg" 
#define CHAT_ID "5616445524"

// --- PINS ---
#define BUTTON_PIN 22     
#define BUZZER_PIN 23    
#define GPS_RX_PIN 16     
#define GPS_TX_PIN 17     
#define PIEZO_PIN 32      // NEW: Piezo Sensor
#define LED_PIN 25        // NEW: Impact Indicator LED

// --- TIMING & THRESHOLDS ---
#define ESCALATION_TIME 45000 
int piezoThreshold = 1000; // Change this based on your stomp tests

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

// SOS Logic Variables
bool sosActive = false;
bool escalated = false;
unsigned long sosStartTime = 0;
unsigned long lastPollTime = 0;
unsigned long lastDebugPrint = 0;

// Wi-Fi Backup Variables
String wifiLat = "0.000000";
String wifiLng = "0.000000";
bool wifiLocAcquired = false;

// Piezo Logic Variables
unsigned long ledTurnOffTime = 0;
bool ledOn = false;

// ====================================================
// CORE FUNCTION: FETCH WI-FI FALLBACK
// ====================================================
void fetchWiFiLocation() {
  Serial.print("[INIT] Fetching Network Backup Location... ");
  HTTPClient http;
  http.begin("http://ip-api.com/json/"); 
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      wifiLat = String((float)doc["lat"], 6);
      wifiLng = String((float)doc["lon"], 6);
      wifiLocAcquired = true;
      Serial.println("SUCCESS. Backup Locked.");
    } else {
      Serial.println("JSON PARSE FAILED.");
    }
  } else {
    Serial.println("HTTP REQUEST FAILED.");
  }
  http.end();
}

// ====================================================
// CORE FUNCTION: GET BEST AVAILABLE LOCATION
// ====================================================
String getBestLocation() {
  String locData = "";
  if (gps.location.isValid()) {
    locData = "http://maps.google.com/maps?q=" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
    locData += "\n*(Source: Precise GPS)*";
  } else if (wifiLocAcquired) {
    locData = "http://maps.google.com/maps?q=" + wifiLat + "," + wifiLng;
    locData += "\n*(Source: Network Estimation - GPS Searching)*";
  } else {
    locData = "Location Unavailable (Searching...)";
  }
  return locData;
}

// ====================================================

void setup() {
  Serial.begin(115200);
  
  // Hardware Setup
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PIEZO_PIN, INPUT); 
  pinMode(LED_PIN, OUTPUT);
  
  digitalWrite(BUZZER_PIN, LOW); 
  digitalWrite(LED_PIN, LOW);

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

  fetchWiFiLocation();

  Serial.print("[INIT] Cleaning old Telegram messages... ");
  int num = botGuardian.getUpdates(botGuardian.last_message_received + 1);
  while(num) {
    num = botGuardian.getUpdates(botGuardian.last_message_received + 1);
  }
  Serial.println("Clean.");
  
  udp.begin(UDP_PORT);
  
  Serial.println("------------------------------------------");
  Serial.println("SYSTEM READY.");
  Serial.println("Piezo Impact System: ARMED (Test Mode)");
  Serial.println("SOS Button System: ARMED");
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
  // 1. Silently Feed GPS Data continuously
  while (GPS_Serial.available() > 0) gps.encode(GPS_Serial.read());

  // 2. PIEZOELECTRIC IMPACT SYSTEM (Thread 2)
  int piezoValue = analogRead(PIEZO_PIN);
  if (piezoValue > piezoThreshold) {
    Serial.print("[PIEZO] Impact Detected! Value: ");
    Serial.println(piezoValue);
    
    digitalWrite(LED_PIN, HIGH);
    ledOn = true;
    ledTurnOffTime = millis() + 500; // Keep LED on for 500ms
  }

  // Shut off LED if time has passed
  if (ledOn && (millis() > ledTurnOffTime)) {
    digitalWrite(LED_PIN, LOW);
    ledOn = false;
  }

  // 3. Debug Print every 5 seconds
  if (millis() - lastDebugPrint > 5000) {
    lastDebugPrint = millis();
    
    Serial.println("--- SYSTEM STATUS ---");
    if (gps.location.isValid()) {
      Serial.print("[LOCATION] SATELLITE | Lat: "); 
      Serial.print(gps.location.lat(), 6);
      Serial.print(", Lng: ");
      Serial.println(gps.location.lng(), 6);
    } else if (wifiLocAcquired) {
      Serial.print("[LOCATION] WI-FI NETWORK | Lat: "); 
      Serial.print(wifiLat);
      Serial.print(", Lng: ");
      Serial.println(wifiLng);
    } else {
      Serial.println("[LOCATION] ERROR: No Network and No GPS.");
    }
    Serial.println("---------------------");
  }

  // 4. Check Panic Button (Thread 1)
  if (digitalRead(BUTTON_PIN) == LOW && !sosActive) {
    delay(50); 
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("\n[EVENT] BUTTON PRESSED! INIT SOS SEQUENCE.");
      
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
      String msg = "🚨 SOS ALERT! 🚨\n\nMap:\n";
      msg += getBestLocation(); 
      msg += "\n\nReply 'SAFE' in 45s to stop Police escalation.";
      
      if(botGuardian.sendMessage(CHAT_ID, msg, "")){
        Serial.println("SENT.");
      } else {
        Serial.println("FAILED.");
      }
    }
  }

  // 5. SOS Active Logic & Escalation
  if (sosActive) {
    if (millis() - lastPollTime > 3000) {
      int num = botGuardian.getUpdates(botGuardian.last_message_received + 1);
      if (num > 0) handleReplies(num);
      lastPollTime = millis();
    }

    if (sosActive && !escalated && (millis() - sosStartTime > ESCALATION_TIME)) {
      Serial.println("\n[TIMEOUT] 45s Passed. Escalating...");
      
      String msg = "⚠️ URGENT: POLICE ASSISTANCE REQUIRED ⚠️\n";
      msg += "Victim Unresponsive / Guardian Timeout.\n\nLAST KNOWN LOCATION:\n";
      msg += getBestLocation();

      botAuthority.sendMessage(CHAT_ID, msg, "");
      
      escalated = true; 
      sosActive = false; 
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("[RESET] System Ready.");
    }
  }
}
