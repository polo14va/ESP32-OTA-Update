#include "FirmwareUpdater.h"
#include <Update.h>

FirmwareUpdater::FirmwareUpdater() : expectedSize(0), writtenSize(0) {}

bool FirmwareUpdater::initialize(uint32_t totalSize) {
    expectedSize = totalSize;
    writtenSize = 0;
    return Update.begin(totalSize);
}

bool FirmwareUpdater::writePacket(const uint8_t* data, size_t length) {
    size_t written = Update.write(data, length);
    if(written != length) {
        return false;
    }
    writtenSize += length;
    return true;
}

bool FirmwareUpdater::finalize() {
    bool success = Update.end(true);
    if(success && Update.isFinished()) {
        return true;
    }
    return false;
}
