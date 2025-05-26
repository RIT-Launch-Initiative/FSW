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

template <typename T>
void benchmarkDataloggerMode(const char *testName, const char *filePath, LogMode mode, size_t maxPackets = 1000, size_t syncFrequency = 10) {
    LOG_INF("\n\n=== %s ===", testName);

    CDataLogger<T> logger(filePath, mode, maxPackets);

    uint64_t totalWriteCycles = 0;
    uint64_t totalSyncCycles = 0;

    uint64_t totalWrites = 0;
    uint64_t totalSyncs = 0;

    uint64_t totalWriteFailures = 0;
    uint64_t totalSyncFailures = 0;


    for (size_t i = 0; i < maxPackets; i++) {
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

            uint64_t elapsedCycles = timing_cycles_get(&start, &end);
            totalWriteCycles += elapsedCycles;
            totalWrites++;
            LOG_INF("\tWrote packet %zu in %llu ns (%llu cycles)", i, timing_cycles_to_ns(elapsedCycles), elapsedCycles);
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
                uint64_t elapsedCycles = timing_cycles_get(&start, &end);
                totalSyncCycles += elapsedCycles;
                totalSyncs++;
                LOG_INF("\tSynchronized after %zu packets in %llu ns (%llu cycles)", i + 1, timing_cycles_to_ns(elapsedCycles), elapsedCycles);
            }
        }
    }

    uint64_t totalWriteTimeNs = timing_cycles_to_ns(totalWriteCycles);
    uint64_t totalSyncTimeNs = timing_cycles_to_ns(totalSyncCycles);

    LOG_INF("Wrote %llu packets of size %zu successfully", totalWrites, sizeof(T));
    if (syncFrequency > 0) {
        LOG_INF("Synchronized %llu times successfully", totalSyncs);
    }

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
    LOG_INF("Benchmark %s completed!\n\n", testName);
}

static void runDataloggerBenchmarks() {
    benchmarkDataloggerMode<SmallPacket>("Small Packet (Growing Mode)", "/lfs/small_growing.bin", LogMode::Growing, 1000, 100);

    benchmarkDataloggerMode<MediumPacket>("Medium Packet (Growing Mode)", "/lfs/medium_growing.bin", LogMode::Growing);

    benchmarkDataloggerMode<LargePacket>("Large Packet (Growing Mode)", "/lfs/large_growing.bin", LogMode::Growing);

    //benchmarkDataloggerMode<MediumPacket>("Medium Packet (Circular Mode)", "/lfs/medium_circular.bin",
    //                                       LogMode::Circular, 500);

    //benchmarkDataloggerMode<MediumPacket>("Medium Packet (FixedSize Mode)", "/lfs/medium_fixed.bin", LogMode::FixedSize,
     //                                      500);
}

int main() {
    printFilesystemStats("/lfs");

    k_msleep(10000);
    timing_init();
    runDataloggerBenchmarks();

    LOG_INF("\n\n=== Final Filesystem Stats ===");
    printFilesystemStats("/lfs");
    LOG_INF("Benchmark completed!");

    while (true) {
        k_msleep(10000);
    }

    return 0;
}
