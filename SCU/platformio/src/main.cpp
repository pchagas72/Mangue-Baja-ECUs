#include <Arduino.h>

// YOUR PINS
#define MODEM_TX 17
#define MODEM_RX 16

// Try to find the pin labeled "PWR" or "RST" on your modem/board
// If you are using a TTGO T-Call, this is usually pin 4 or 23.
// If you are using a generic Red SIM800 module, you might not need this.
#define MODEM_PWRKEY 4 

void setup() {
  Serial.begin(115200);
  
  // Try 9600 first (most common factory default)
  // If this doesn't work, change to 115200 and re-upload.
  Serial2.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
  
  Serial.println("--- ESP32 <-> SIM800 Bridge ---");
  Serial.println("Type 'AT' and press Enter. If no reply, check wiring.");

  // MANUAL POWER ON SEQUENCE (Blindly trying to wake it up)
  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, LOW); // Pull Low for 1s to turn ON
  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(3000); // Wait for boot
}

void loop() {
  // Pass data from Computer -> Modem
  if (Serial.available()) {
    Serial2.write(Serial.read());
  }
  
  // Pass data from Modem -> Computer
  if (Serial2.available()) {
    Serial.write(Serial2.read());
  }
}
