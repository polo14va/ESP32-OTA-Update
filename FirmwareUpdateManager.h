#ifndef FIRMWARE_UPDATE_MANAGER_H
#define FIRMWARE_UPDATE_MANAGER_H

#include <Arduino.h>
#include "BLEFirmwareService.h"
#include "FirmwareUpdater.h"

class FirmwareUpdateManager : public IFirmwareUpdateHandler {
public:
    FirmwareUpdateManager();
    void begin(bool enableUpdate);
    void onHandshakeReceived(uint32_t totalSize, uint16_t totalPackets) override;
    void onDataReceived(const uint8_t* data, size_t length) override;
private:
    BLEFirmwareService* bleService;
    FirmwareUpdater* updater;
    uint16_t totalPackets;
    uint16_t currentPacket;
};

#endif
