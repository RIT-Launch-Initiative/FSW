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
    uint32_t filenameHash; // Hash filename as a way to check for metadata
    char filename[31];
    uint8_t version;
    size_t packetSize;
    size_t allocatedSize;
    size_t nextFileAddress;
};

constexpr uint32_t DATALOGGER_VERSION = 1;

template <typename T, size_t numPacketsInBuffer>
class CRawDataLogger {
public:
    CRawDataLogger(const device* flashDev, off_t flashAddress, off_t fileSize, const std::string& filename,
                   DataloggerMode mode)
        : flash(flashDev), flashAddress(flashAddress), fileSize(fileSize), mode(mode), lastError(0),
          initialized(false), currentOffset(sizeof(DataloggerMetadata)), nextFileAddress(0) {
        resetBuffers();

        int ret = flash_get_size(flash, &flashSize);
        if (ret < 0) {
            lastError = ret;
            initialized = false;
        }

        ret = prepMetadata(filename, 0); // next_file_address = 0 for first file
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


    }

    bool IsInitialized() const { return initialized; }

    DataloggerMode GetMode() const { return mode; }

    int Write(const T& data, bool flush = false) {
        if (!initialized) return lastError ? lastError : -1;
        size_t spaceLeft = fileSize - currentOffset;

        if (spaceLeft < sizeof(T)) {
            resetBuffers();

            int ret = 0;

            switch (mode) {
                case DataloggerMode::Fixed:
                    return -ENOSPC;
                case DataloggerMode::LinkedFixed:
                case DataloggerMode::LinkedTruncate:
                    printk("Creating new file!\n");
                    seekAndUpdateMetadata();
                    ret = stream_flash_init(&ctx, flash, buffer, sizeof(buffer), nextFileAddress, fileSize, nullptr);
                    int ret = stream_flash_buffered_write(&ctx, reinterpret_cast<const uint8_t*>(&metadata),
                                                          sizeof(metadata), true);
                    if (ret < 0) {
                        lastError = ret;
                        return ret;
                    }
                    ret = stream_flash_init(&ctx, flash, buffer, sizeof(buffer), nextFileAddress + sizeof(metadata),
                                      fileSize - sizeof(metadata), nullptr);
                    flashAddress = nextFileAddress;
                    currentOffset = sizeof(metadata);
                    break;
            }

            if (ret < 0) {
                lastError = ret;
                printk("Error reinitializing: %d\n", ret);
                return ret;
            }
        } else if ((spaceLeft - sizeof(T)) < sizeof(T)) { // Force flush if this write will fill the file
            flush = true;
            printk("Forced flush!\n");
        }

        int ret = stream_flash_buffered_write(&ctx, reinterpret_cast<const uint8_t*>(&data), sizeof(T), flush);
        if (ret < 0) {
            printk("Failed to write data: %d\n", ret);
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
        size_t addr = flashAddress + fileSize;
        size_t fileSz = fileSize;
        DataloggerMetadata meta{};

        while (addr + sizeof(DataloggerMetadata) < flashSize) {
            int ret = readMetadata(addr, meta);
            printk("Reading metadata at 0x%zx. Got %d\n", addr, ret);

            if (ret == 0) {
                // Metadata found, jump to next file boundary
                addr += meta.allocatedSize;
            } else {
                // No metadata found
                if (mode == DataloggerMode::LinkedFixed) {
                    // Check if enough space for a new file
                    for (size_t testAddr = addr; testAddr < addr + fileSz; testAddr += sizeof(DataloggerMetadata)) {
                        ret = readMetadata(testAddr, meta);
                        if (ret == 0) {
                            // Found metadata in the range, cannot use this space
                            addr = testAddr + fileSz; // Move to next possible file start
                            break;
                        }

                        if (testAddr + sizeof(DataloggerMetadata) >= addr + fileSz) {
                            return std::make_pair(addr, fileSz);
                        }
                    }
                } else if (mode == DataloggerMode::LinkedTruncate) {
                    // Truncate to next metadata or use initial size
                    size_t next_meta_addr = findNextMetadata(addr + sizeof(DataloggerMetadata), flashSize);
                    size_t available_size = (next_meta_addr > addr) ? (next_meta_addr - addr) : fileSz;
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
    uint8_t buffer[numPacketsInBuffer * sizeof(T)];
    DataloggerMode mode;
    int lastError;
    bool initialized;
    DataloggerMetadata metadata;
    size_t currentOffset;
    size_t nextFileAddress;

    int prepMetadata(const std::string& filename, size_t nextAddr) {
        memset(&metadata, 0, sizeof(metadata));
        strncpy(metadata.filename, filename.c_str(), sizeof(metadata.filename) - 1);
        metadata.allocatedSize = fileSize;
        metadata.version = DATALOGGER_VERSION;
        metadata.packetSize = sizeof(T);
        metadata.nextFileAddress = nextAddr;
        metadata.filenameHash = sys_hash32_murmur3(metadata.filename, sizeof(metadata.filename));

        size_t len = sizeof(metadata);

        // DO NOT WRITE nextFileAddress IF ZERO AND MODE IS LINKED. This is so we can write nextAddr later, without an erase
        if (nextAddr == 0 && (mode == DataloggerMode::LinkedTruncate || mode == DataloggerMode::LinkedFixed)) {
            len -= sizeof(metadata.nextFileAddress); //
        }

        return flash_write(flash, flashAddress, &metadata, len);
    }

    int linkToNextFile(size_t addr) {
        if (mode != DataloggerMode::LinkedFixed && mode != DataloggerMode::LinkedTruncate) return -1;
        return flash_write(flash, flashAddress + offsetof(DataloggerMetadata, nextFileAddress),
                           &addr, sizeof(metadata.nextFileAddress));
    }

    int readMetadata(off_t addr, DataloggerMetadata& outMeta) {
        while (addr + sizeof(DataloggerMetadata) < flashSize) {
            int ret = flash_read(flash, addr, &outMeta, sizeof(DataloggerMetadata));
            if (ret < 0) {
                printk("Failed to read metadata at 0x%zx\n", addr);
                return ret;
            }

            if (outMeta.filenameHash == sys_hash32_murmur3(outMeta.filename, sizeof(outMeta.filename))) {
                return 0;
            }

            addr += sizeof(DataloggerMetadata);
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

    void seekAndUpdateMetadata() {
        auto spaceOpt = FindLinkedSpace();
        if (!spaceOpt.has_value()) {
            lastError = -ENOSPC;
            return;
        }
        size_t newAddr = spaceOpt.value().first;
        size_t newSize = spaceOpt.value().second;
        printk("New file at 0x%zx of size 0x%zx\n", newAddr, newSize);

        int ret = linkToNextFile(newAddr);
        if (ret < 0) {
            lastError = ret;
            return;
        }
        printk("Linked!");

        // Write new metadata
        flashAddress = newAddr; // Update flash address to new file
        ret = prepMetadata(metadata.filename, 0);
        if (ret < 0) {
            lastError = ret;
            return;
        }
        printk("Wrote new metadata!");

        currentOffset = newAddr + sizeof(metadata);
    }
};

#endif //C_RAW_DATALOGGER_H