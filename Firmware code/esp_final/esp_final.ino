#include <esp_camera.h>
#include <SPI.h>
#include <MFRC522.h>
#include "FS.h"
#include "SPIFFS.h"
#include <WiFi.h>
#include <HTTPClient.h>

// // RFID Pins
// #define RST_PIN 15
// #define SDA_PIN 14
// MFRC522 rfid(SDA_PIN, RST_PIN);

// Camera pins (for OV2640)
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

// Signal from UNO
#define SIGNAL_FROM_UNO 12
#define SIGNAL_TO_UNO_VALID 13
#define SIGNAL_TO_UNO_INVALID 14

// LED Flash pin
#define FLASH_LED_PIN     4 // onboard flash LED (GPIO 4)

// WiFi credentials
const char* ssid = ".";        
const char* password = "1234567890";         

// Server URL
String serverURL = "http://192.168.252.37:5000/upload"; // <<< your server IP

void setup() {
  Serial.begin(115200);

  // Init Flash LED
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  // UNO Signals
  pinMode(SIGNAL_TO_UNO_VALID, OUTPUT);
  pinMode(SIGNAL_TO_UNO_INVALID, OUTPUT);
  pinMode(SIGNAL_FROM_UNO, INPUT); // Corrected to INPUT
  digitalWrite(SIGNAL_TO_UNO_VALID, LOW);
  digitalWrite(SIGNAL_TO_UNO_INVALID, LOW);

  // // Init RFID
  // SPI.begin(12, 2, 13, 14); // SCK=12, MISO=2, MOSI=13, SS=14
  // rfid.PCD_Init();
  // Serial.println("RFID Reader Initialized");

  // Init SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    while (true) delay(1000);
  }
  Serial.println("SPIFFS Mounted");

  // Init Camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera Init Failed");
    while (true) delay(1000);
  }
  Serial.println("Camera Initialized");

  // Init WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (digitalRead(SIGNAL_FROM_UNO) == HIGH) {
    delay(50); // 🔥 small debounce delay (optional but improves stability)

    Serial.println("✅ Valid RFID Detected!");

    // Turn ON Flash
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(200); // small delay for LED stabilization

    // Capture Image
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      digitalWrite(FLASH_LED_PIN, LOW);
      return;
    }

    // Send Image to Server
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverURL);
      http.addHeader("Content-Type", "image/jpeg");

      int httpResponseCode = http.POST(fb->buf, fb->len);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Server Response: " + response);

        if (response == "true") {
          Serial.println("✅ Face Authorized - Open Door");
          digitalWrite(FLASH_LED_PIN, LOW);
          digitalWrite(SIGNAL_TO_UNO_VALID, HIGH);
          delay(5000);
          digitalWrite(SIGNAL_TO_UNO_VALID, LOW);
        } else {
          Serial.println("❌ Face Unauthorized - Access Denied");
          digitalWrite(FLASH_LED_PIN, LOW);
          digitalWrite(SIGNAL_TO_UNO_INVALID, HIGH);
          delay(1000);
          digitalWrite(SIGNAL_TO_UNO_INVALID, LOW);
        }
      } else {
        Serial.println("Error in HTTP POST");
        
      }

      http.end();
    } else {
      Serial.println("WiFi not connected");
    }

    esp_camera_fb_return(fb); // Release frame buffer
  }

  delay(100); // Check every 100 ms
}
