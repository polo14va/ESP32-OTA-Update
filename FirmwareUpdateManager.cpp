#include "FirmwareUpdateManager.h"

FirmwareUpdateManager::FirmwareUpdateManager()
    : bleService(this),
      totalPackets(0),
      currentPacket(0),
      updateModeActive(false) {
}

void FirmwareUpdateManager::begin(bool enableUpdate) {
    if (!enableUpdate || updateModeActive) return;
    updateModeActive = true;
    bleService.begin();
}

void FirmwareUpdateManager::onHandshakeReceived(uint32_t totalSize, uint16_t totalPackets) {
    this->totalPackets = totalPackets;
    currentPacket = 0;
    updater.initialize(totalSize);
}

void FirmwareUpdateManager::onDataReceived(const uint8_t* data, size_t length) {
    currentPacket++;
    bool success = updater.writePacket(data, length);
    bleService.sendAck(currentPacket, success);
    if (currentPacket == totalPackets) {
        updater.finalize();
        updateModeActive = false;
    }
}

void FirmwareUpdateManager::startUpdateMode() {
    begin(true);
}