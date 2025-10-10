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
    char logName[15];
    uint8_t version;
    size_t packetSize;
    size_t allocatedSize;
    size_t nextLogAddress;
};

constexpr uint32_t DATALOGGER_VERSION = 1;

template <typename T, size_t numPacketsInBuffer>
class CRawDataLogger {
public:
    /**
     * Constructor
     * @param flashDev Flash device to write to
     * @param flashAddress Starting address in flash to write log
     * @param logSize Total size of log including metadata
     * @param logName Name of the log (for metadata)
     * @param mode Datalogger mode (Fixed, LinkedFixed, LinkedTruncate)
     */
    explicit CRawDataLogger(const device* flashDev, const off_t flashAddress, const off_t logSize, const std::string& logName, const DataloggerMode mode)
        : flash(flashDev), mode(mode), originalLogSize(logSize), lastError(0), initialized(false), flashAddress(flashAddress),
          currentLogSize(logSize), currentOffset(sizeof(DataloggerMetadata)), nextLogAddress(0) {
        resetBuffers();

        int ret = flash_get_size(flash, &flashSize);
        if (ret < 0) {
            lastError = ret;
            initialized = false;
        }

        ret = prepMetadata(logName);
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

    /**
     * Destructor ensuring any buffered data is flushed
     * This is more for testing, since this lifetime of this class will most likely be forever in real-world applications
     */
    ~CRawDataLogger() {
        if (initialized) {
            stream_flash_buffered_write(&ctx, nullptr, 0, true);
        }
    }

    /**
     * Write data to the buffer which will flush to flash as needed
     * @param data Data to write
     * @return 0 on success, negative errno code on failure
     */
    int Write(const T& data) {
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
                    ret = seekAndUpdateMetadata();
                    if (ret < 0) {
                        lastError = ret;
                        return ret;
                    }

                    ret = stream_flash_init(&ctx, flash, buffer, sizeof(buffer), flashAddress + sizeof(metadata),
                                      currentLogSize - sizeof(metadata), nullptr);
                    if (ret < 0) {
                        lastError = ret;
                        return ret;
                    }

                    currentOffset = sizeof(metadata);
            }
        }

        const bool flush = (spaceLeft - sizeof(T)) < sizeof(T);
        int ret = stream_flash_buffered_write(&ctx, reinterpret_cast<const uint8_t*>(&data), sizeof(T), flush);
        if (ret < 0) {
            lastError = ret;
            return ret;
        }
        currentOffset += sizeof(T);
        return 0;
    }


    /**
     * Getter to check if the datalogger was initialized successfully
     * @return true if initialized, false otherwise
     */
    bool IsInitialized() const { return initialized; }

    /**
     * Getter for last error code
     * @return Last error code encountered, 0 if no error
     */
    int GetLastError() const { return lastError; }

private:
    const device* flash;
    const DataloggerMode mode;
    const size_t originalLogSize;
    uint64_t flashSize{};

    int lastError;
    bool initialized;

    uint8_t buffer[numPacketsInBuffer * sizeof(T)]{};
    stream_flash_ctx ctx{};

    DataloggerMetadata metadata{};
    size_t flashAddress;
    size_t currentLogSize;
    size_t currentOffset;
    size_t nextLogAddress;

    /**
     *
     * @param logName Log name for metadata
     * @return
     */
    int prepMetadata(const std::string& logName) {
        memset(&metadata, 0, sizeof(metadata));
        strncpy(metadata.logName, logName.c_str(), sizeof(metadata.logName) - 1);
        metadata.allocatedSize = currentLogSize;
        metadata.version = DATALOGGER_VERSION;
        metadata.packetSize = sizeof(T);
        metadata.nextLogAddress = 0;
        metadata.logNameHash = sys_hash32_murmur3(metadata.logName, sizeof(metadata.logName));

        // DO NOT WRITE nextLogAddress IF ZERO AND MODE IS LINKED. This is so we can write nextAddr later, without an erase
        const size_t len = sizeof(metadata) - sizeof(metadata.logNameHash);

        return flash_write(flash, flashAddress, &metadata, len);
    }

    /**
     * Link the current log to the next log by writing the address into the current log's metadata
     * @param addr Address to link to as the next log
     * @return 0 on success, negative errno code on failure, -1 if mode is not linked
     */
    int linkToNextLog(const size_t addr) {
        if (mode != DataloggerMode::LinkedFixed && mode != DataloggerMode::LinkedTruncate) return -1;
        return flash_write(flash, flashAddress + offsetof(DataloggerMetadata, nextLogAddress),
                           &addr, sizeof(metadata.nextLogAddress));
    }

    /**
     * Attempt to read metadata from flash and validate it
     * @param addr Address to read metadata from
     * @param outMeta Output metadata structure
     * @return 0 on success, negative errno code on failure, -1 if metadata is invalid (bad hash)
     */
    int readMetadata(const off_t addr, DataloggerMetadata& outMeta) {
        int ret = flash_read(flash, addr, &outMeta, sizeof(DataloggerMetadata));
        if (ret < 0) {
            return ret;
        }

        if (outMeta.logNameHash == sys_hash32_murmur3(outMeta.logName, sizeof(outMeta.logName))) {
            return 0;
        }

        return -1;
    }

    /**
     * Linear search for the next valid metadata structure in flash
     * @param startAddr Address to start searching from
     * @param maxAddr Address to stop searching at
     * @return Address of next valid metadata, or maxAddr if none found
     */
    size_t findNextMetadata(const off_t startAddr, const off_t maxAddr) {
        DataloggerMetadata meta{};
        for (off_t addr = startAddr; static_cast<off_t>(addr + sizeof(DataloggerMetadata)) < maxAddr;
            addr += sizeof(DataloggerMetadata)) {
            if (readMetadata(addr, meta) == 0) {
                return addr;
            }
        }
        return maxAddr;
    }

    /**
     * Reset internal buffers and context
     */
    void resetBuffers() {
        memset(&ctx, 0, sizeof(ctx));
        memset(&buffer, 0, sizeof(buffer));
    }

    /**
     * Find the next available space for a linked log
     * @return Optional pair of (address, size) for the next linked log space, or nullopt if none found or not in linked mode
     */
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

    /**
     * Seek to the next available linked log space and update metadata accordingly
     * @return 0 on success, negative errno code on failure
     */
    int seekAndUpdateMetadata() {
        auto spaceOpt = findLinkedSpace();
        if (!spaceOpt.has_value()) {
            lastError = -ENOSPC;
            return -ENOSPC;
        }

        size_t newAddr = spaceOpt.value().first;
        size_t newSize = spaceOpt.value().second;

        int ret = linkToNextLog(newAddr);
        if (ret < 0) {
            lastError = ret;
            return ret;
        }

        // Set up new log
        flashAddress = newAddr; // Update flash address to new log
        currentLogSize = newSize;
        ret = prepMetadata(metadata.logName);
        if (ret < 0) {
            lastError = ret;
            return ret;
        }

        currentOffset = sizeof(metadata);
        return 0;
    }
};

#endif //C_RAW_DATALOGGER_H