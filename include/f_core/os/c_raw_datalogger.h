#ifndef C_RAW_DATALOGGER_H
#define C_RAW_DATALOGGER_H

#include <zephyr/storage/stream_flash.h>
#include <cstring>
#include <string>
#include <optional>
#include <zephyr/kernel.h>
#include <zephyr/sys/hash_function.h>

enum class DataloggerMode {
    Fixed,
    LinkedFixed,
    LinkedTruncate
};

struct DataloggerMetadata {
    uint32_t logNameHash; // Hash logName as a way to check for metadata
    char logName[31];
    uint8_t version;
    size_t packetSize;
    size_t allocatedSize;
    size_t nextLogAddress;
};

constexpr uint32_t DATALOGGER_VERSION = 1;

template <typename T, size_t numPacketsInBuffer>
class CRawDataLogger {
public:
    CRawDataLogger(const device* flashDev, off_t flashAddress, off_t logSize, const std::string& logName,
                   DataloggerMode mode)
        : flash(flashDev), flashAddress(flashAddress), originalLogSize(logSize), currentLogSize(logSize), mode(mode), lastError(0),
          initialized(false), currentOffset(sizeof(DataloggerMetadata)), nextLogAddress(0) {
        resetBuffers();

        int ret = flash_get_size(flash, &flashSize);
        if (ret < 0) {
            lastError = ret;
            initialized = false;
        }

        ret = prepMetadata(logName, 0); // next_log_address = 0 for first log
        if (ret < 0) {
            lastError = ret;
            return;
        }

        initialized = true;
        currentOffset = sizeof(metadata);

        ret = stream_flash_init(&ctx, flash, buffer, sizeof(buffer), flashAddress + sizeof(metadata),
                                logSize - sizeof(metadata), nullptr);
        if (ret < 0) {
            lastError = ret;
            initialized = false;
        }
    }

    ~CRawDataLogger() {
        if (initialized) {
            stream_flash_buffered_write(&ctx, nullptr, 0, true);
        }
    }

    bool IsInitialized() const { return initialized; }

    DataloggerMode GetMode() const { return mode; }

    int Write(const T& data, bool flush = false) {
        if (!initialized) return lastError ? lastError : -1;
        size_t spaceLeft = currentLogSize - currentOffset;

        if (spaceLeft < sizeof(T)) {
            resetBuffers();

            int ret = 0;

            switch (mode) {
                case DataloggerMode::Fixed:
                    return -ENOSPC;
                case DataloggerMode::LinkedFixed:
                case DataloggerMode::LinkedTruncate:
                    seekAndUpdateMetadata();

                    ret = stream_flash_init(&ctx, flash, buffer, sizeof(buffer), flashAddress + sizeof(metadata),
                                      currentLogSize - sizeof(metadata), nullptr);
                    if (ret < 0) {
                        lastError = ret;
                        return ret;
                    }

                    currentOffset = sizeof(metadata);
            }
        } else if ((spaceLeft - sizeof(T)) < sizeof(T)) { // Force flush if this write will fill the log
            flush = true;
        }

        int ret = stream_flash_buffered_write(&ctx, reinterpret_cast<const uint8_t*>(&data), sizeof(T), flush);
        if (ret < 0) {
            lastError = ret;
            return ret;
        }
        currentOffset += sizeof(T);
        return 0;
    }

    size_t GetBytesWritten() const {
        return stream_flash_bytes_written(&ctx);
    }


    int GetLastError() const { return lastError; }
    off_t GetCurrentOffset() const { return currentOffset; }
    off_t GetNextLogAddress() const { return nextLogAddress; }

private:
    const device* flash;
    stream_flash_ctx ctx;
    size_t flashAddress;
    uint64_t flashSize;
    const size_t originalLogSize;
    size_t currentLogSize;
    uint8_t buffer[numPacketsInBuffer * sizeof(T)];
    DataloggerMode mode;
    int lastError;
    bool initialized;
    DataloggerMetadata metadata;
    size_t currentOffset;
    size_t nextLogAddress;

    int prepMetadata(const std::string& logName, size_t nextAddr) {
        memset(&metadata, 0, sizeof(metadata));
        strncpy(metadata.logName, logName.c_str(), sizeof(metadata.logName) - 1);
        metadata.allocatedSize = currentLogSize;
        metadata.version = DATALOGGER_VERSION;
        metadata.packetSize = sizeof(T);
        metadata.nextLogAddress = nextAddr;
        metadata.logNameHash = sys_hash32_murmur3(metadata.logName, sizeof(metadata.logName));

        size_t len = sizeof(metadata);

        // DO NOT WRITE nextLogAddress IF ZERO AND MODE IS LINKED. This is so we can write nextAddr later, without an erase
        if (nextAddr == 0 && (mode == DataloggerMode::LinkedTruncate || mode == DataloggerMode::LinkedFixed)) {
            len -= sizeof(metadata.nextLogAddress); //
        }

        return flash_write(flash, flashAddress, &metadata, len);
    }

    int linkToNextLog(size_t addr) {
        if (mode != DataloggerMode::LinkedFixed && mode != DataloggerMode::LinkedTruncate) return -1;
        return flash_write(flash, flashAddress + offsetof(DataloggerMetadata, nextLogAddress),
                           &addr, sizeof(metadata.nextLogAddress));
    }

    int readMetadata(off_t addr, DataloggerMetadata& outMeta) {
        int ret = flash_read(flash, addr, &outMeta, sizeof(DataloggerMetadata));
        if (ret < 0) {
            return ret;
        }

        if (outMeta.logNameHash == sys_hash32_murmur3(outMeta.logName, sizeof(outMeta.logName))) {
            return 0;
        }

        return -1;
    }

    size_t findNextMetadata(off_t startAddr, off_t maxAddr) {
        DataloggerMetadata meta{};
        for (off_t addr = startAddr; static_cast<off_t>(addr + sizeof(DataloggerMetadata)) < maxAddr;
            addr += sizeof(DataloggerMetadata)) {
            if (readMetadata(addr, meta) == 0) {
                return addr;
            }
        }
        return maxAddr;
    }

    int resetBuffers() {
        memset(&ctx, 0, sizeof(ctx));
        memset(&buffer, 0, sizeof(buffer));

        return 0;
    }

    std::optional<std::pair<size_t, size_t>> findLinkedSpace() {
        if (mode != DataloggerMode::LinkedFixed && mode != DataloggerMode::LinkedTruncate) return std::nullopt;
        size_t addr = flashAddress + currentLogSize;
        size_t logSz = originalLogSize;
        DataloggerMetadata meta{};

        while (addr + sizeof(DataloggerMetadata) < flashSize) {
            int ret = readMetadata(addr, meta);

            if (ret == 0) {
                // Metadata found, jump to next log boundary
                addr += meta.allocatedSize;
            } else {
                // No metadata found
                // Check if enough space for a new log
                for (size_t testAddr = addr; testAddr < addr + logSz; testAddr += 0x10) {
                    if (readMetadata(testAddr, meta) == 0) {
                        if (mode == DataloggerMode::LinkedTruncate) {
                            logSz = (testAddr - addr); // Shrink log size to fit
                            return std::make_pair(addr, MIN(logSz, flashSize - addr));
                        }

                        // Found metadata in the range, cannot use this space
                        addr = testAddr + meta.allocatedSize; // Move to next possible log start
                        break;
                    }
                }
                return std::make_pair(addr, logSz);
            }
        }
        return std::nullopt;
    }

    void seekAndUpdateMetadata() {
        auto spaceOpt = findLinkedSpace();
        if (!spaceOpt.has_value()) {
            lastError = -ENOSPC;
            return;
        }
        size_t newAddr = spaceOpt.value().first;
        size_t newSize = spaceOpt.value().second;

        int ret = linkToNextLog(newAddr);
        if (ret < 0) {
            lastError = ret;
            return;
        }

        // Set up new log
        flashAddress = newAddr; // Update flash address to new log
        currentLogSize = newSize;
        ret = prepMetadata(metadata.logName, 0);
        if (ret < 0) {
            lastError = ret;
            return;
        }

        currentOffset = sizeof(metadata);
    }
};

#endif //C_RAW_DATALOGGER_H