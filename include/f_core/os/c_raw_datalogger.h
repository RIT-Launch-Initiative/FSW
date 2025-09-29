#ifndef C_RAW_DATALOGGER_H
#define C_RAW_DATALOGGER_H

#include <zephyr/storage/stream_flash.h>
#include <cstring>
#include <string>
#include <optional>

enum class DataloggerMode {
    Rotating,
    Fixed,
    LinkedFixed,
    LinkedTruncate
};

struct DataloggerMetadata {
    char filename[32];
    size_t allocated_size;
    uint32_t version;
    size_t packet_size;
    size_t next_file_address;
};

constexpr uint32_t DATALOGGER_VERSION = 1;

template <typename T, size_t packetBufferSize>
class CRawDataLogger {
public:
    CRawDataLogger(const device* flashDev, off_t flashAddress, off_t fileSize, const std::string& filename,
                   DataloggerMode mode)
        : flash(flashDev), flashAddress(flashAddress), fileSize(fileSize), mode(mode), lastError(0),
          initialized(false), currentOffset(0), nextFileAddress(0) {
        memset(&ctx, 0, sizeof(ctx));
        memset(buffer, 0, sizeof(buffer));

        PrepMetadata(filename, 0); // next_file_address = 0 for first file
        int ret = stream_flash_init(&ctx, flash, buffer, sizeof(buffer), flashAddress, fileSize, nullptr);
        if (ret < 0) {
            lastError = ret;
            return;
        }
        ret = stream_flash_buffered_write(&ctx, reinterpret_cast<const uint8_t*>(&metadata), sizeof(metadata), true);
        if (ret < 0) {
            lastError = ret;
            return;
        }
        initialized = true;
        currentOffset = sizeof(metadata);
        ret = stream_flash_init(&ctx, flash, buffer, sizeof(buffer), flashAddress + sizeof(metadata),
                                fileSize - sizeof(metadata), nullptr);
        if (ret < 0) {
            lastError = ret;
            initialized = false;
        }

        ret = flash_get_size(flash, &flashSize);
        if (ret < 0) {
            lastError = ret;
            initialized = false;
        }
    }

    bool IsInitialized() const { return initialized; }
    DataloggerMode GetMode() const { return mode; }

    int Write(const T& data, bool flush = false) {
        if (!initialized) return lastError ? lastError : -1;
        size_t bytesWritten = stream_flash_bytes_written(&ctx);
        size_t spaceLeft = fileSize - sizeof(metadata) - bytesWritten;
        if (spaceLeft < sizeof(T)) {
            switch (mode) {
                case DataloggerMode::Rotating:
                    currentOffset = sizeof(metadata);
                    stream_flash_init(&ctx, flash, buffer, sizeof(buffer), flashAddress + sizeof(metadata),
                                      fileSize - sizeof(metadata), nullptr);
                    break;
                case DataloggerMode::Fixed:
                    return -ENOSPC;
                case DataloggerMode::LinkedFixed:
                case DataloggerMode::LinkedTruncate:
                    nextFileAddress = flashAddress + fileSize;
                    PrepMetadata(metadata.filename, nextFileAddress);
                    stream_flash_init(&ctx, flash, buffer, sizeof(buffer), nextFileAddress, fileSize, nullptr);
                    int ret = stream_flash_buffered_write(&ctx, reinterpret_cast<const uint8_t*>(&metadata),
                                                          sizeof(metadata), true);
                    if (ret < 0) {
                        lastError = ret;
                        return ret;
                    }
                    stream_flash_init(&ctx, flash, buffer, sizeof(buffer), nextFileAddress + sizeof(metadata),
                                      fileSize - sizeof(metadata), nullptr);
                    flashAddress = nextFileAddress;
                    currentOffset = sizeof(metadata);
                    break;
            }
        }
        int ret = stream_flash_buffered_write(&ctx, reinterpret_cast<const uint8_t*>(&data), sizeof(T), flush);
        if (ret < 0) {
            lastError = ret;
            return ret;
        }
        currentOffset += sizeof(T);
        return 0;
    }

    int Flush() {
        if (!initialized) {
            return (lastError != 0) ? lastError : -1;
        }

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

    std::optional<std::pair<size_t, size_t>> FindLinkedSpace() {
        if (mode != DataloggerMode::LinkedFixed && mode != DataloggerMode::LinkedTruncate) return std::nullopt;
        size_t addr = flashAddress;
        size_t file_sz = fileSize;
        DataloggerMetadata meta;
        while (addr + sizeof(DataloggerMetadata) < flashSize) {
            int ret = ReadMetadata(addr, meta);
            if (ret == 0) {
                // Metadata found, follow next pointer or jump to next file boundary
                if (meta.next_file_address != 0 && meta.next_file_address > addr) {
                    addr = meta.next_file_address;
                } else {
                    addr += file_sz;
                }
            } else {
                // No metadata found
                if (mode == DataloggerMode::LinkedFixed) {
                    // Check if enough space for a new file
                    if (addr + file_sz < flashSize) {
                        return std::make_pair(addr, file_sz);
                    }
                } else if (mode == DataloggerMode::LinkedTruncate) {
                    // Truncate to next metadata or use initial size
                    size_t next_meta_addr = FindNextMetadata(addr + sizeof(DataloggerMetadata), flashSize);
                    size_t available_size = (next_meta_addr > addr) ? (next_meta_addr - addr) : file_sz;
                    return std::make_pair(addr, available_size);
                }
            }
        }
        return std::nullopt;
    }


    int GetLastError() const { return lastError; }
    off_t GetCurrentOffset() const { return currentOffset; }
    off_t GetNextFileAddress() const { return nextFileAddress; }

private:
    const device* flash;
    stream_flash_ctx ctx;
    size_t flashAddress;
    uint64_t flashSize;
    size_t fileSize;
    uint8_t buffer[packetBufferSize];
    DataloggerMode mode;
    int lastError;
    bool initialized;
    DataloggerMetadata metadata;
    size_t currentOffset;
    size_t nextFileAddress;

    void PrepMetadata(const std::string& filename, size_t nextAddr) {
        memset(&metadata, 0, sizeof(metadata));
        strncpy(metadata.filename, filename.c_str(), sizeof(metadata.filename) - 1);
        metadata.allocated_size = fileSize;
        metadata.version = DATALOGGER_VERSION;
        metadata.packet_size = sizeof(T);
        metadata.next_file_address = nextAddr;
    }

    int ReadMetadata(off_t addr, DataloggerMetadata& outMeta) {
        while (addr + sizeof(DataloggerMetadata) < flashSize) {
            int ret = flash_read(flash, addr, &outMeta, sizeof(DataloggerMetadata));
            if (ret < 0) {
                return ret;
            }
            if (outMeta.version == DATALOGGER_VERSION && outMeta.packet_size == sizeof(T) &&
                outMeta.allocated_size == fileSize) {
                return 0;
            }
            addr += sizeof(DataloggerMetadata);
        }
        return -1;
    }

    size_t FindNextMetadata(off_t startAddr, off_t maxAddr) {
        DataloggerMetadata meta{};
        for (off_t addr = startAddr; addr + sizeof(DataloggerMetadata) < maxAddr;
             addr += sizeof(DataloggerMetadata)) {
            if (ReadMetadata(addr, meta) == 0) {
                return addr;
            }
        }
        return maxAddr;
    }
};

#endif //C_RAW_DATALOGGER_H