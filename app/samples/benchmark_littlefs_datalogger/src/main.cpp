#include "f_core/os/c_datalogger.h"

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/timing/timing.h>

LOG_MODULE_REGISTER(benchmark, LOG_LEVEL_INF);

// Test data structures
struct SmallPacket {
    uint32_t timestamp;
    float x;
    float y;
    float z;
};

struct MediumPacket {
    uint32_t timestamp;

    float accelX;
    float accelY;
    float accelZ;

    float gyroX;
    float gyroY;
    float gyroZ;

    float magX;
    float magY;
    float magZ;

    float temperature;
    float pressure;
    uint8_t status;
};

struct LargePacket {
    uint32_t timestamp;
    float sensorData[32];
};

// Benchmark configuration
constexpr size_t numIterations = 1000;

// Utility functions
void printSize(const char *label, size_t size) {
    if (size < 1024) {
        LOG_INF("%s: %zu B", label, size);
    } else if (size < 1024 * 1024) {
        LOG_INF("%s: %.2f KiB", label, ((double) size) / 1024);
    } else {
        LOG_INF("%s: %.2f MiB", label, ((double) size) / (1024 * 1024));
    }
}

void printFilesystemStats(const char *mountPoint) {
    struct fs_statvfs stat;
    int ret = fs_statvfs(mountPoint, &stat);

    if (ret == 0) {
        LOG_INF("Filesystem stats for %s:", mountPoint);
        LOG_INF("  Block size: %lu bytes", stat.f_bsize);
        LOG_INF("  Total blocks: %lu", stat.f_blocks);
        LOG_INF("  Free blocks: %lu", stat.f_bfree);
        printSize("Total Space", stat.f_blocks * stat.f_bsize);
        printSize("Free Space", stat.f_bfree * stat.f_bsize);
    } else {
        LOG_ERR("Failed to get filesystem stats: %d", ret);
    }
}

static uint64_t fileCreationLoop(const char *files[], size_t numFiles) {
    size_t filesCreated = 0;

    timing_t start = 0;
    timing_t end = 0;
    uint64_t totalCycles = 0;
    uint64_t totalTimeNs = 0;

    LOG_INF("\tFile Creation:");

    for (size_t i = 0; i < numFiles; ++i) {
        fs_file_t file;

        timing_start();
        start = timing_counter_get();
        int ret = fs_open(&file, files[i], FS_O_CREATE | FS_O_RDWR);
        end = timing_counter_get();
        timing_stop();

        if (ret < 0) {
            LOG_ERR("\t\tFailed to create file %s: %d", files[i], ret);
            continue;
        }

        uint64_t elapsedCycles = timing_cycles_get(&start, &end);
        uint64_t elapsedNs = timing_cycles_to_ns(elapsedCycles);
        totalCycles += elapsedCycles;
        totalTimeNs += elapsedNs;
        filesCreated++;
        LOG_INF("\t\tCreated file %s in %llu ns", files[i], elapsedNs);

        fs_close(&file);
    }

    LOG_INF("Created %zu files in %u ms (%u cycles)", filesCreated, totalTimeNs / 1000000, totalCycles);

    return totalTimeNs / 1000000;
}

static uint64_t fileDeletionLoop(const char *files[], size_t numFiles) {
    size_t filesDeleted = 0;

    timing_t start = 0;
    timing_t end = 0;
    uint64_t totalCycles = 0;
    uint64_t totalTimeNs = 0;

    LOG_INF("\tFile Deletion:");

    for (size_t i = 0; i < numFiles; ++i) {
        timing_start();
        start = timing_counter_get();
        int ret = fs_unlink(files[i]);
        end = timing_counter_get();
        timing_stop();

        if (ret < 0) {
            LOG_ERR("\t\tFailed to delete file %s: %d", files[i], ret);
            continue;
        }

        uint64_t elapsedCycles = timing_cycles_get(&start, &end);
        uint64_t elapsedNs = timing_cycles_to_ns(elapsedCycles);
        totalCycles += elapsedCycles;
        totalTimeNs += elapsedNs;
        filesDeleted++;
        LOG_INF("\t\tDeleted file %s in %llu ns", files[i], elapsedNs);
    }

    LOG_INF("Deleted %zu files in %u ms (%u cycles)", filesDeleted, totalTimeNs / 1000000, totalCycles);

    return totalTimeNs / 1000000;
}


void benchmarkRawFilesystem(const char *testName, const char *filePath, LogMode mode, size_t maxPackets = 0) {
    LOG_INF("=== %s ===", testName);

    const char *files[] = {
        "/lfs/1", "/lfs/2", "/lfs/3", "/lfs/4", "/lfs/5",
    };

    uint64_t totalCreationTime = fileCreationLoop(files, sizeof(files) / sizeof(files[0]));
    LOG_INF("Total file creation time: %llu ms", totalCreationTime);

    uint64_t totalDeletionTime = fileDeletionLoop(files, sizeof(files) / sizeof(files[0]));
    LOG_INF("Total file deletion time: %llu ms", totalDeletionTime);

    fs_dirent st;
    int ret = fs_stat(filePath, &st);
    if (ret == 0) {
        printSize("Final File Size", st.size);
    }
}

template <typename T>
void benchmarkDataloggerMode(const char *testName, const char *filePath, LogMode mode, size_t maxPackets = 0, int syncFrequency = 0) {
    LOG_INF("=== %s ===", testName);

    CDataLogger<T> logger(filePath, mode, maxPackets);

    uint64_t totalWriteCycles = 0;
    uint64_t totalSyncCycles = 0;

    uint64_t totalWrites = 0;
    uint64_t totalSyncs = 0;

    uint64_t totalWriteFailures = 0;
    uint64_t totalSyncFailures = 0;


    for (size_t i = 0; i < numIterations; i++) {
        T packet;


        // Fill packet
        {
            packet.timestamp = k_uptime_get_32();
            for (size_t j = 0; j < sizeof(packet) / sizeof(float); ++j) {
                reinterpret_cast<float *>(&packet)[j] = static_cast<float>(j + i);
            }
        }

        timing_t start = 0;
        timing_t end = 0;

        // Time a write
        {
            timing_start();
            start = timing_counter_get();
            int ret = logger.Write(packet);
            end = timing_counter_get();
            timing_stop();

            if (ret < 0) {
                LOG_ERR("Failed to write packet %zu: %d", i, ret);
                totalWriteFailures++;
                continue;
            }

            totalWriteCycles += timing_cycles_get(&start, &end);
            totalWrites++;
        }

        // Time a sync if necessary
        if (syncFrequency > 0 && (i + 1) % syncFrequency == 0) {
            timing_start();
            start = timing_counter_get();
            int ret = logger.Sync();
            end = timing_counter_get();
            timing_stop();

            if (ret < 0) {
                LOG_ERR("Failed to sync after %zu packets: %d", i + 1, ret);
                totalSyncFailures++;
                continue;
            } else {
                totalSyncCycles += timing_cycles_get(&start, &end);
                totalSyncs++;
            }
        }


        if (syncFrequency > 0 && (i + 1) % syncFrequency == 0) {
            logger.Sync();
            LOG_INF("Synchronized after %zu packets", i + 1);
        }
    }

    uint64_t totalWriteTimeNs = timing_cycles_to_ns(totalWriteCycles);
    uint64_t totalSyncTimeNs = timing_cycles_to_ns(totalSyncCycles);

    LOG_INF("Wrote %zu / %zu packets successfully", totalWrites, numIterations);
    LOG_INF("Synced %zu / %zu times", totalSyncs, numIterations / syncFrequency);

    LOG_INF("Total write time: %llu ns (%llu cycles)", totalWriteTimeNs, totalWriteCycles);
    LOG_INF("Total sync time: %llu ns (%llu cycles)", totalSyncTimeNs, totalSyncCycles);

    LOG_INF("Average write time: %.2f ns", totalWrites > 0 ? (double)totalWriteTimeNs / totalWrites : 0.0);
    LOG_INF("Average sync time: %.2f ns", totalSyncs > 0 ? (double)totalSyncTimeNs / totalSyncs : 0.0);

    fs_dirent st;
    int ret = fs_stat(filePath, &st);
    if (ret == 0) {
        printSize("Final File Size", st.size);
    } else {
        LOG_ERR("Failed to get file stats: %d", ret);
    }
    LOG_INF("Benchmark %s completed!", testName);
}

static void runDataloggerBenchmarks() {
    benchmarkDataloggerMode<SmallPacket>("Small Packet (Growing Mode)", "/lfs/small_growing.bin", LogMode::Growing);

    // benchmarkDataloggerMode<MediumPacket>("Medium Packet (Growing Mode)", "/lfs/medium_growing.bin", LogMode::Growing);
    //
    // benchmarkDataloggerMode<LargePacket>("Large Packet (Growing Mode)", "/lfs/large_growing.bin", LogMode::Growing);
    //
    // benchmarkDataloggerMode<MediumPacket>("Medium Packet (Circular Mode)", "/lfs/medium_circular.bin",
    //                                       LogMode::Circular, 500);
    //
    // benchmarkDataloggerMode<MediumPacket>("Medium Packet (FixedSize Mode)", "/lfs/medium_fixed.bin", LogMode::FixedSize,
    //                                       500);
}

int main() {
    LOG_INF("LittleFS and CDataLogger Benchmark Starting...");
    LOG_INF("Iterations: %u", numIterations);

    k_msleep(100);
    printFilesystemStats("/lfs");

    runDataloggerBenchmarks();

    printFilesystemStats("/lfs");
    LOG_INF("Benchmark completed!");

    while (true) {
        k_msleep(10000);
    }

    return 0;
}
