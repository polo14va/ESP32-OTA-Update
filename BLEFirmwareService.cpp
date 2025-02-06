#include "BLEFirmwareService.h"

BLEFirmwareService::BLEFirmwareService(IFirmwareUpdateHandler* updateHandler)
    : updateHandler(updateHandler), pServer(nullptr), pService(nullptr),
      handshakeChar(nullptr), dataChar(nullptr), ackChar(nullptr) {}

void BLEFirmwareService::begin() {
    BLEDevice::init("OTA_Device");
    pServer = BLEDevice::createServer();
    pService = pServer->createService(BLEUUID("0000FFF0-0000-1000-8000-00805F9B34FB"));
    handshakeChar = pService->createCharacteristic(
        BLEUUID("0000FFF1-0000-1000-8000-00805F9B34FB"),
        BLECharacteristic::PROPERTY_WRITE
    );
    dataChar = pService->createCharacteristic(
        BLEUUID("0000FFF2-0000-1000-8000-00805F9B34FB"),
        BLECharacteristic::PROPERTY_WRITE
    );
    ackChar = pService->createCharacteristic(
        BLEUUID("0000FFF3-0000-1000-8000-00805F9B34FB"),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pService->start();
    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(BLEUUID("0000FFF0-0000-1000-8000-00805F9B34FB"));
    advertising->setScanResponse(false);
    BLEDevice::startAdvertising();
}

void BLEFirmwareService::sendAck(uint16_t packetNumber, bool status) {
    char ackMessage[16];
    snprintf(ackMessage, sizeof(ackMessage), "%u:%d", packetNumber, status);
    ackChar->setValue((uint8_t*)ackMessage, strlen(ackMessage));
    ackChar->notify();
}