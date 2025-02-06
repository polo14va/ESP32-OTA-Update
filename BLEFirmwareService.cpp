#include "BLEFirmwareService.h"
#include <NimBLEDevice.h>
#include <NimBLEDescriptor.h>
#include <string.h>

void BLEFirmwareService::FirmwareUpdateCallback::onWrite(NimBLECharacteristic* pCharacteristic) {
  std::string value = pCharacteristic->getValue();
  size_t dataLength = value.length();
  if (dataLength > 0) {
    const uint8_t* rxData = reinterpret_cast<const uint8_t*>(value.data());
    uint8_t command = rxData[0];
    switch (command) {
      case 0xFD:
        break;
      case 0xFE:
        if (dataLength >= 5) {
          uint32_t totalSize = ((uint32_t)rxData[1] << 24) |
                               ((uint32_t)rxData[2] << 16) |
                               ((uint32_t)rxData[3] << 8)  |
                               ((uint32_t)rxData[4]);
          bleService->fileSize = totalSize;
        }
        break;
      case 0xFF:
        if (dataLength >= 3) {
          uint16_t totalPackets = ((uint16_t)rxData[1] << 8) | (uint16_t)rxData[2];
          bleService->updateHandler->onHandshakeReceived(bleService->fileSize, totalPackets);
        }
        break;
      case 0xFB: {
        if (dataLength > 2) {
          size_t chunkLength = dataLength - 2;
          if (bleService->currentChunkSize + chunkLength <= sizeof(bleService->currentChunk)) {
            memcpy(bleService->currentChunk + bleService->currentChunkSize, rxData + 2, chunkLength);
            bleService->currentChunkSize += chunkLength;
          }
        }
        break;
      }
      case 0xFC:
        if (bleService->currentChunkSize > 0) {
          bleService->updateHandler->onDataReceived(bleService->currentChunk, bleService->currentChunkSize);
          bleService->currentChunkSize = 0;
        }
        break;
      case 0xF2:
        break;
      default:
        break;
    }
  }
}

BLEFirmwareService::BLEFirmwareService(IFirmwareUpdateHandler* updateHandler)
  : updateHandler(updateHandler),
    pServer(nullptr),
    pService(nullptr),
    rxChar(nullptr),
    txChar(nullptr),
    fileSize(0),
    currentChunkSize(0),
    firmwareCallback(this) {
}

void BLEFirmwareService::begin() {
  NimBLEDevice::init("OTA_Device");
  pServer = NimBLEDevice::createServer();
  pService = pServer->createService("fb1e4000-54ae-4a28-9f74-dfccb248601d");
  rxChar = pService->createCharacteristic("fb1e4002-54ae-4a28-9f74-dfccb248601d", NIMBLE_PROPERTY::WRITE);
  rxChar->setCallbacks(&firmwareCallback);
  txChar = pService->createCharacteristic("fb1e4003-54ae-4a28-9f74-dfccb248601d", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  txChar->addDescriptor(new NimBLEDescriptor(NimBLEUUID((uint16_t)0x2902),
                                              NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE,
                                              2,
                                              nullptr));
  pService->start();
  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID("fb1e4000-54ae-4a28-9f74-dfccb248601d");
  NimBLEAdvertisementData srData;
  advertising->setScanResponseData(srData);
  NimBLEDevice::startAdvertising();
  currentChunkSize = 0;
}

void BLEFirmwareService::sendAck(uint16_t packetNumber, bool status) {
  uint8_t ackPacket[2] = {0xAA, status ? 0x01 : 0x00};
  txChar->setValue(ackPacket, 2);
  txChar->notify();
}
