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

// Datalogger benchmark
template <typename T>
void benchmarkDataloggerMode(const char *testName, const char *filePath, LogMode mode, size_t maxPackets = 0) {
    LOG_INF("=== %s ===", testName);


    fs_dirent st;
    int ret = fs_stat(filePath, &st);
    if (ret == 0) {
        printSize("Final File Size", st.size);
    }
}

static void runDataloggerBenchmarks() {
    benchmarkDataloggerMode<SmallPacket>("Small Packet (Growing Mode)", "/lfs/small_growing.bin", LogMode::Growing);

    benchmarkDataloggerMode<MediumPacket>("Medium Packet (Growing Mode)", "/lfs/medium_growing.bin", LogMode::Growing);

    benchmarkDataloggerMode<LargePacket>("Large Packet (Growing Mode)", "/lfs/large_growing.bin", LogMode::Growing);

    benchmarkDataloggerMode<MediumPacket>("Medium Packet (Circular Mode)", "/lfs/medium_circular.bin",
                                          LogMode::Circular, 500);

    benchmarkDataloggerMode<MediumPacket>("Medium Packet (FixedSize Mode)", "/lfs/medium_fixed.bin", LogMode::FixedSize,
                                          500);
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
