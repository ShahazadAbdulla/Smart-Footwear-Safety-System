#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "FS.h"
#include "SD_MMC.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ================= CONFIGURATION =================
const char* ssid = "Wifi";       
const char* password = "password";
#define RECORD_DURATION 30       

// Wireless
WiFiUDP udp;
const int UDP_PORT = 4210;
char packetBuffer[255];

// Pins (AI Thinker)
#define FLASH_GPIO_NUM 4
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

bool isRecording = false;
unsigned long recordEndTime = 0;
File recordFile;

void configCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA; 
  config.jpeg_quality = 12; 
  config.fb_count = 2; 
  config.grab_mode = CAMERA_GRAB_LATEST;
  esp_camera_init(&config);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);
  pinMode(FLASH_GPIO_NUM, OUTPUT);
  digitalWrite(FLASH_GPIO_NUM, LOW);
  configCamera();
  if(!SD_MMC.begin("/sdcard", true)) return;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  udp.begin(UDP_PORT);
}

void startRecording() {
  String path = "/SOS_LATEST.mjpeg";
  if(SD_MMC.exists(path)) SD_MMC.remove(path);
  recordFile = SD_MMC.open(path.c_str(), FILE_WRITE);
  if (!recordFile) return;
  digitalWrite(FLASH_GPIO_NUM, HIGH); 
  isRecording = true;
  recordEndTime = millis() + (RECORD_DURATION * 1000);
}

void stopRecording() {
  if(recordFile) recordFile.close();
  isRecording = false;
  digitalWrite(FLASH_GPIO_NUM, LOW); 
}

void loop() {
  if (!isRecording) {
    int packetSize = udp.parsePacket();
    if (packetSize) {
      int len = udp.read(packetBuffer, 255);
      if (len > 0) packetBuffer[len] = 0;
      if (String(packetBuffer) == "START_REC") startRecording();
    }
  }
  if (isRecording) {
    if (millis() > recordEndTime) stopRecording();
    else {
      camera_fb_t * fb = esp_camera_fb_get();
      if (fb) {
        recordFile.write(fb->buf, fb->len);
        esp_camera_fb_return(fb);
      }
    }
  }
}
