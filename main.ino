#include <Arduino.h>
#include "FirmwareUpdateManager.h"

FirmwareUpdateManager firmwareUpdateManager;

void setup() {
  Serial.begin(115200);
  firmwareUpdateManager.begin(true);
  Serial.println("Device ready. Send 'update' via Serial to activate update mode.");
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command.equalsIgnoreCase("update")) {
      Serial.println("Activating update mode...");
      firmwareUpdateManager.startUpdateMode();
    }
  }
  delay(100);
}
