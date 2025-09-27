#ifndef C_RAW_DATALOGGER_H
#define C_RAW_DATALOGGER_H

#include <zephyr/storage/stream_flash.h>
#include <cstring>
#include <string>

struct DataloggerMetadata {
    char filename[32];
    size_t allocated_size;
    uint32_t version;
    size_t packet_size;
};

constexpr uint32_t DATALOGGER_VERSION = 1;

template <typename T, size_t packetBufferSize>
class CRawDataLogger {
public:
    CRawDataLogger(const device *flash_dev, size_t flashAddress, size_t fileSize, const std::string &filename)
        : flash(flash_dev), flashAddress(flashAddress), fileSize(fileSize), lastError(0), initialized(false)
    {
        // prep metadata
        DataloggerMetadata meta = {};
        strncpy(meta.filename, filename.c_str(), sizeof(meta.filename) - 1);
        meta.allocated_size = fileSize;
        meta.version = DATALOGGER_VERSION;
        meta.packet_size = sizeof(T);
        int ret = stream_flash_init(&ctx, flash, buffer, sizeof(buffer), flashAddress, fileSize, nullptr);
        if (ret < 0) {
            lastError = ret;
            return;
        }
        ret = stream_flash_buffered_write(&ctx, reinterpret_cast<const uint8_t*>(&meta), sizeof(meta), true);
        if (ret < 0) {
            lastError = ret;
            return;
        }
        initialized = true;
        ret = stream_flash_init(&ctx, flash, buffer, sizeof(buffer), flashAddress + sizeof(meta), fileSize - sizeof(meta), nullptr);
        if (ret < 0) {
            lastError = ret;
            initialized = false;
        }
    }

    bool IsInitialized() const { return initialized; }

    int Write(const T &data, bool flush = false) {
        if (!initialized) return lastError ? lastError : -1;
        int ret = stream_flash_buffered_write(&ctx, reinterpret_cast<const uint8_t*>(&data), sizeof(T), flush);
        if (ret < 0) {
            lastError = ret;
            return ret;
        }
        return 0;
    }

    int Flush() {
        if (!initialized) return lastError ? lastError : -1;
        int ret = stream_flash_buffered_write(&ctx, nullptr, 0, true);
        if (ret < 0) {
            lastError = ret;
            return ret;
        }
        return 0;
    }

    size_t GetBytesWritten() const {
        return stream_flash_bytes_written(&ctx);
    }
    size_t GetBytesBuffered() const {
        return stream_flash_bytes_buffered(&ctx);
    }
    int GetLastError() const { return lastError; }

private:
    const device *flash;
    stream_flash_ctx ctx;
    size_t flashAddress;
    size_t fileSize;
    uint8_t buffer[packetBufferSize];
    int lastError;
    bool initialized;
};

#endif //C_RAW_DATALOGGER_H