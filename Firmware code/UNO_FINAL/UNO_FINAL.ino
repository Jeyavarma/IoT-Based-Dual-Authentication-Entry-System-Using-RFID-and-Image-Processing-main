#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

// RFID Pins
#define SS_PIN 10
#define RST_PIN 9

// LEDs and Servo
#define GREEN_LED 7
#define RED_LED 6
#define SERVO_PIN 8

// Communication Pins with ESP32
#define SIGNAL_TO_ESP 5
#define SIGNAL_FROM_ESP_VALID 4
#define SIGNAL_FROM_ESP_INVALID 3

MFRC522 rfid(SS_PIN, RST_PIN); // Create MFRC522 instance
Servo myServo;

// Valid UIDs
const String validUIDs[] = {
  "bd993902",  // Add more UIDs if needed
  "096f776c",
  "b99e4262"
};
const int numOfValidUIDs = sizeof(validUIDs) / sizeof(validUIDs[0]);

void setup() {
  Serial.begin(9600);
  Serial.println("Hello, system starting...");
  
  SPI.begin();        // Init SPI bus
  rfid.PCD_Init();    // Init MFRC522

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  pinMode(SIGNAL_TO_ESP, OUTPUT);
  pinMode(SIGNAL_FROM_ESP_VALID, INPUT);
  pinMode(SIGNAL_FROM_ESP_INVALID, INPUT);

  digitalWrite(SIGNAL_TO_ESP, LOW); // Initially LOW
  
  myServo.attach(SERVO_PIN);
  lockServo(); // Start locked
  
  Serial.println("Place your card on the reader...");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String scannedUID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) scannedUID += "0";
    scannedUID += String(rfid.uid.uidByte[i], HEX);
  }
  scannedUID.toLowerCase(); // Ensure lowercase

  Serial.print("Scanned UID: ");
  Serial.println(scannedUID);

  if (isValidUID(scannedUID)) {
    Serial.println("✅ UID Authorized");

    // Tell ESP32 to capture image and verify
    digitalWrite(SIGNAL_TO_ESP, HIGH);
    delay(500); // Increased delay to ensure proper signal detection
    digitalWrite(SIGNAL_TO_ESP, LOW);
    
    // Now wait for ESP32 response
    bool waitForResponse = true;
    unsigned long startTime = millis();
    const unsigned long timeout = 5000; // 5 sec timeout

    while (waitForResponse) {
      if (digitalRead(SIGNAL_FROM_ESP_VALID) == HIGH) {
        Serial.println("✅ Face Authorized by ESP32");
        digitalWrite(RED_LED, LOW);
        digitalWrite(GREEN_LED, HIGH);
        unlockServo();
        delay(2000); // Keep door open for 2 seconds
        lockServo();
        digitalWrite(GREEN_LED, LOW);
        waitForResponse = false;
      }
      else if (digitalRead(SIGNAL_FROM_ESP_INVALID) == HIGH) {
        Serial.println("❌ Face Not Authorized by ESP32");
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RED_LED, HIGH);
        lockServo();
        delay(2000);
        digitalWrite(RED_LED, LOW);
        waitForResponse = false;
      }

      // Timeout if ESP32 doesn't respond
      if (millis() - startTime > timeout) {
        Serial.println("⚠️ Timeout: No response from ESP32");
        lockServo();
        waitForResponse = false;
      }
    }

  } else {
    Serial.println("❌ UID Not Authorized");
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    lockServo();
    delay(2000);
    digitalWrite(RED_LED, LOW);
  }

  rfid.PICC_HaltA();      // Halt PICC
  rfid.PCD_StopCrypto1(); // Stop encryption on PCD
  rfid.uid.size = 0;      // Clear UID buffer
}

bool isValidUID(String uid) {
  for (int i = 0; i < numOfValidUIDs; i++) {
    if (uid == validUIDs[i]) {
      return true;
    }
  }
  return false;
}

void unlockServo() {
  myServo.write(90); // Unlock position
}

void lockServo() {
  myServo.write(0); // Locked position
}
