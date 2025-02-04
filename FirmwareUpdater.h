#ifndef FIRMWARE_UPDATER_H
#define FIRMWARE_UPDATER_H

#include <Arduino.h>

class FirmwareUpdater {
public:
    FirmwareUpdater();
    bool initialize(uint32_t totalSize);
    bool writePacket(const uint8_t* data, size_t length);
    bool finalize();
private:
    uint32_t expectedSize;
    uint32_t writtenSize;
};

#endif
