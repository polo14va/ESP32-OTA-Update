#ifndef BLE_FIRMWARE_SERVICE_H
#define BLE_FIRMWARE_SERVICE_H

#include <Arduino.h>
#include <NimBLEDevice.h>

class IFirmwareUpdateHandler {
public:
  virtual void onHandshakeReceived(uint32_t totalSize, uint16_t totalPackets) = 0;
  virtual void onDataReceived(const uint8_t* data, size_t length) = 0;
};

class BLEFirmwareService {
public:
  class FirmwareUpdateCallback : public NimBLECharacteristicCallbacks {
  public:
    FirmwareUpdateCallback(BLEFirmwareService* service) : bleService(service) {}
    void onWrite(NimBLECharacteristic* pCharacteristic);
  private:
    BLEFirmwareService* bleService;
  };

  BLEFirmwareService(IFirmwareUpdateHandler* updateHandler);
  void begin();
  void sendAck(uint16_t packetNumber, bool status);

  uint32_t fileSize;
  uint8_t currentChunk[256];
  size_t currentChunkSize;
  IFirmwareUpdateHandler* updateHandler;

private:
  NimBLEServer* pServer;
  NimBLEService* pService;
  NimBLECharacteristic* rxChar;
  NimBLECharacteristic* txChar;
  FirmwareUpdateCallback firmwareCallback;
};

#endif
