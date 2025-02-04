#ifndef BLE_FIRMWARE_SERVICE_H
#define BLE_FIRMWARE_SERVICE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

class IFirmwareUpdateHandler {
public:
    virtual void onHandshakeReceived(uint32_t totalSize, uint16_t totalPackets) = 0;
    virtual void onDataReceived(const uint8_t* data, size_t length) = 0;
};

class BLEFirmwareService {
public:
    BLEFirmwareService(IFirmwareUpdateHandler* updateHandler);
    void begin();
    void sendAck(uint16_t packetNumber, bool status);
private:
    IFirmwareUpdateHandler* updateHandler;
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* handshakeChar;
    BLECharacteristic* dataChar;
    BLECharacteristic* ackChar;
};

#endif
