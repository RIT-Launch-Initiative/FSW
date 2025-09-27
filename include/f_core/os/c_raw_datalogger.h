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
        : flash(flash_dev), flashAddress(flashAddress), fileSize(fileSize), bytesBuffered(0), bytesWritten(0), initialized(false)
    {
        memset(&ctx, 0, sizeof(ctx));
        memset(buffer, 0, sizeof(buffer));

        // prep metadata
        DataloggerMetadata meta = {};
        strncpy(meta.filename, filename.c_str(), sizeof(meta.filename) - 1);
        meta.allocated_size = fileSize;
        meta.version = DATALOGGER_VERSION;
        meta.packet_size = sizeof(T);

        // metadata to flash
        int ret = stream_flash_init(&ctx, flash, buffer, bufferSize, flashAddress, fileSize, nullptr);
        if (ret < 0) {
            lastError = ret;
            return;
        }
        ret = stream_flash_buffered_write(&ctx, reinterpret_cast<const uint8_t*>(&meta), sizeof(meta), true);
        if (ret < 0) {
            lastError = ret;
            return;
        }
        bytesWritten = sizeof(meta);
        initialized = true;

        // reinit ctx start after metadata
        ret = stream_flash_init(&ctx, flash, buffer, bufferSize, flashAddress + sizeof(meta), fileSize - sizeof(meta), nullptr);
        if (ret < 0) {
            lastError = ret;
            initialized = false;
        }
    }

    bool IsInitialized() const { return initialized; }

    int Write(const T &data, bool flush = false) {
        if (!initialized) return lastError ? lastError : -1;

        if (bytesBuffered + sizeof(T) > bufferSize) {
            int ret = Flush();
            if (ret < 0) return ret;
        }

        memcpy(buffer + bytesBuffered, &data, sizeof(T));
        bytesBuffered += sizeof(T);

        // Flush if requested or buffer full
        if (flush || bytesBuffered == bufferSize) {
            int ret = Flush();
            if (ret < 0) return ret;
        }
        return 0;
    }

    int Flush() {
        if (!initialized) return lastError ? lastError : -1;
        if (bytesBuffered == 0) return 0;
        int ret = stream_flash_buffered_write(&ctx, buffer, bytesBuffered, true);
        if (ret < 0) {
            lastError = ret;
            return ret;
        }
        bytesWritten += bytesBuffered;
        bytesBuffered = 0;
        return 0;
    }

    size_t GetBytesWritten() const { return bytesWritten; }
    size_t GetBytesBuffered() const { return bytesBuffered; }

private:
    const device *flash;
    stream_flash_ctx ctx;
    size_t flashAddress;
    size_t fileSize;
    uint8_t buffer[packetBufferSize * sizeof(T)];
    size_t bufferSize = sizeof(buffer);
    size_t bytesBuffered;
    size_t bytesWritten;
    int lastError = 0;
    bool initialized;
};

#endif //C_RAW_DATALOGGER_H