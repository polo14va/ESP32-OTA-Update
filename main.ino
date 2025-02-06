#include <Arduino.h>
#include "FirmwareUpdateManager.h"

FirmwareUpdateManager firmwareUpdateManager;
#define LED_PIN 2

void setup() {
  Serial.begin(115200);
  Serial.println(F("Device ready. Send 'update' via Serial to activate update mode."));
  firmwareUpdateManager.startUpdateMode();
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command.equalsIgnoreCase(F("update"))) {
      Serial.println(F("Activating update mode..."));
      firmwareUpdateManager.startUpdateMode();
    }
  }
  delay(100);

  static unsigned long previousMillis = 0;
  if (millis() - previousMillis >= 2000) {
    previousMillis = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}
