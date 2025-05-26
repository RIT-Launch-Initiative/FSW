#include "f_core/os/c_datalogger.h"

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#ifdef CONFIG_TIMING_FUNCTIONS
#include <zephyr/timing/timing.h>
#endif
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(benchmark, LOG_LEVEL_INF);

// Test data structures
struct SmallPacket {
    uint32_t timestamp;
    uint16_t sensor_id;
    float value;
};

struct MediumPacket {
    uint32_t timestamp;
    uint16_t sensor_id;
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    float mag_x, mag_y, mag_z;
    float temperature;
    float pressure;
    uint8_t status;
};

struct LargePacket {
    uint32_t timestamp;
    uint16_t sensor_id;
    float sensor_data[32];
    uint8_t metadata[64];
};

// Filesystem mount point
FS_FSTAB_DECLARE_ENTRY(DT_NODELABEL(lfs1));
fs_mount_t *lfs_mnt_pt = &FS_FSTAB_ENTRY(DT_NODELABEL(lfs1));

// Benchmark configuration
constexpr size_t NUM_ITERATIONS = 1000;
constexpr size_t TIMING_WARMUP = 10;

// Utility functions
void printSize(const char *str, size_t size) {
    if (size < 1024) {
        LOG_INF("%s: %zu B", str, size);
    } else if (size < 1024 * 1024) {
        LOG_INF("%s: %.2f KiB", str, ((double) size) / 1024);
    } else {
        LOG_INF("%s: %.2f MiB", str, ((double) size) / (1024 * 1024));
    }
}

void printFilesystemStats(const char* mount_point) {
    struct fs_statvfs fs_stat;
    int ret = fs_statvfs(mount_point, &fs_stat);
    if (ret == 0) {
        LOG_INF("Filesystem stats for %s:", mount_point);
        LOG_INF("  Block size: %lu bytes", fs_stat.f_bsize);
        LOG_INF("  Total blocks: %lu", fs_stat.f_blocks);
        LOG_INF("  Free blocks: %lu", fs_stat.f_bfree);
        printSize("Total Space", fs_stat.f_blocks * fs_stat.f_bsize);
        printSize("Free Space", fs_stat.f_bfree * fs_stat.f_bsize);
    } else {
        LOG_ERR("Failed to get filesystem stats: %d", ret);
    }
}

template<typename T>
void benchmarkDataloggerMode(const char* test_name, const char* filename, 
                              LogMode mode, size_t max_packets = 0) {
    LOG_INF("=== %s ===", test_name);
    
    // Create test data
    T test_packet;
    memset(&test_packet, 0xAA, sizeof(T));  // Fill with pattern
    
    // Initialize datalogger
    CDataLogger<T> logger(filename, mode, max_packets);
    
#ifdef CONFIG_TIMING_FUNCTIONS
    timing_t startTime, endTime;
    uint64_t total_cycles = 0;
#endif
    uint32_t startMs, endMs;
    
    // Warmup
    for (size_t i = 0; i < TIMING_WARMUP; i++) {
        logger.Write(test_packet);
    }
    
    // Benchmark write operations
#ifdef CONFIG_TIMING_FUNCTIONS
    timing_start();
    startTime = timing_counter_get();
#endif
    startMs = k_uptime_get_32();
    
    for (size_t i = 0; i < NUM_ITERATIONS; i++) {
        test_packet.timestamp = i;  // Assuming first member is timestamp
        logger.Write(test_packet);
    }
    
    endMs = k_uptime_get_32();
#ifdef CONFIG_TIMING_FUNCTIONS
    endTime = timing_counter_get();
    total_cycles = timing_cycles_get(&startTime, &endTime);
    timing_stop();
#endif
    
    // Sync and close
#ifdef CONFIG_TIMING_FUNCTIONS
    timing_start();
    startTime = timing_counter_get();
#endif
    uint32_t sync_startMs = k_uptime_get_32();
    
    logger.Sync();
    logger.Close();
    
    uint32_t sync_endMs = k_uptime_get_32();
#ifdef CONFIG_TIMING_FUNCTIONS
    endTime = timing_counter_get();
    uint64_t sync_cycles = timing_cycles_get(&startTime, &endTime);
    timing_stop();
#endif
    
    // Calculate performance metrics
    uint32_t totalMs = endMs - startMs;
    uint32_t syncMs = sync_endMs - sync_startMs;
    double avg_writeMs = (double)totalMs / NUM_ITERATIONS;
    
    size_t total_bytes = NUM_ITERATIONS * sizeof(T);
    double throughputMbps = ((double)total_bytes) / (totalMs / 1000.0) / (1024 * 1024);
    
    LOG_INF("Packet size: %zu bytes", sizeof(T));
    LOG_INF("Total write time: %u ms", totalMs);
    LOG_INF("Average write time: %.3f ms", avg_writeMs);
    LOG_INF("Sync time: %u ms", syncMs);
    printSize("Total Data Written", total_bytes);
    LOG_INF("Write throughput: %.2f MiB/s", throughputMbps);
    LOG_INF("Write rate: %.0f packets/sec", (double)NUM_ITERATIONS * 1000.0 / totalMs);
    LOG_INF("\n\n");

#ifdef CONFIG_TIMING_FUNCTIONS
    uint64_t total_ns = timing_cycles_to_ns(total_cycles);
    uint64_t sync_ns = timing_cycles_to_ns(sync_cycles);
    uint64_t avg_write_ns = total_ns / NUM_ITERATIONS;
    double precise_throughput = ((double)total_bytes * 1000.0) / (total_ns / 1000000.0) / (1024 * 1024);
    
    LOG_INF("Precise timing (cycles):");
    LOG_INF("  Total write time: %llu ns (%.2f ms)", total_ns, total_ns / 1000000.0);
    LOG_INF("  Average write time: %llu ns (%.2f us)", avg_write_ns, avg_write_ns / 1000.0);
    LOG_INF("  Sync time: %llu ns (%.2f ms)", sync_ns, sync_ns / 1000000.0);
    LOG_INF("  Precise throughput: %.2f MiB/s", precise_throughput);
    LOG_INF("  Precise write rate: %.0f packets/sec", (double)NUM_ITERATIONS * 1000000000.0 / total_ns);
#endif
    
    // Check file stats
    fs_dirent stat_dst;
    int ret = fs_stat(filename, &stat_dst);
    if (ret == 0) {
        printSize("Final File Size", stat_dst.size);
    }
}

void benchmarkRawFilesystem() {
    LOG_INF("=== Raw Filesystem Benchmark ===");
    
    const char* test_file = "/lfs/raw_test.bin";
    fs_file_t file;
    uint8_t buffer[1024];
    memset(buffer, 0xAA, sizeof(buffer));
    
    int ret = fs_open(&file, test_file, FS_O_CREATE | FS_O_WRITE);
    if (ret < 0) {
        LOG_ERR("Failed to open file: %d", ret);
        return;
    }
    
    // Write benchmark
#ifdef CONFIG_TIMING_FUNCTIONS
    timing_t startTime, endTime;
    timing_start();
    startTime = timing_counter_get();
#endif
    uint32_t startMs = k_uptime_get_32();
    
    for (size_t i = 0; i < NUM_ITERATIONS; i++) {
        ret = fs_write(&file, buffer, sizeof(buffer));
        if (ret < 0) {
            LOG_ERR("Write failed at iteration %zu: %d", i, ret);
            break;
        }
    }
    
    uint32_t endMs = k_uptime_get_32();
#ifdef CONFIG_TIMING_FUNCTIONS
    endTime = timing_counter_get();
    uint64_t write_cycles = timing_cycles_get(&startTime, &endTime);
    timing_stop();
#endif
    
    // Sync benchmark
#ifdef CONFIG_TIMING_FUNCTIONS
    timing_start();
    startTime = timing_counter_get();
#endif
    uint32_t sync_startMs = k_uptime_get_32();
    
    fs_sync(&file);
    
    uint32_t sync_endMs = k_uptime_get_32();
#ifdef CONFIG_TIMING_FUNCTIONS
    endTime = timing_counter_get();
    uint64_t sync_cycles = timing_cycles_get(&startTime, &endTime);
    timing_stop();
#endif
    
    fs_close(&file);
    
    // Results
    uint32_t writeMs = endMs - startMs;
    uint32_t syncMs = sync_endMs - sync_startMs;
    size_t total_bytes = NUM_ITERATIONS * sizeof(buffer);
    double throughputMbps = ((double)total_bytes) / (writeMs / 1000.0) / (1024 * 1024);
    
    LOG_INF("Raw filesystem write performance:");
    LOG_INF("Buffer size: %zu bytes", sizeof(buffer));
    LOG_INF("Total write time: %u ms", writeMs);
    LOG_INF("Sync time: %u ms", syncMs);
    LOG_INF("Write throughput: %.2f MiB/s", throughputMbps);

#ifdef CONFIG_TIMING_FUNCTIONS
    uint64_t write_ns = timing_cycles_to_ns(write_cycles);
    uint64_t sync_ns = timing_cycles_to_ns(sync_cycles);
    double precise_throughput = ((double)total_bytes * 1000.0) / (write_ns / 1000000.0) / (1024 * 1024);
    
    LOG_INF("Precise timing:");
    LOG_INF("  Total write time: %llu ns (%.2f ms)", write_ns, write_ns / 1000000.0);
    LOG_INF("  Sync time: %llu ns (%.2f ms)", sync_ns, sync_ns / 1000000.0);
    LOG_INF("  Precise throughput: %.2f MiB/s", precise_throughput);
#endif
    LOG_INF("");
    
    // Cleanup
    fs_unlink(test_file);
}

void benchmarkFileOperations() {
    LOG_INF("=== File Operations Benchmark ===");
    
    const char* testFiles[] = {
        "/lfs/test1.bin", "/lfs/test2.bin", "/lfs/test3.bin",
        "/lfs/test4.bin", "/lfs/test5.bin"
    };
    const size_t numFiles = sizeof(testFiles) / sizeof(testFiles[0]);
    
    // File creation benchmark
#ifdef CONFIG_TIMING_FUNCTIONS
    timing_t startTime, endTime;
    timing_start();
    startTime = timing_counter_get();
#endif
    uint32_t createStartMs = k_uptime_get_32();
    
    for (size_t i = 0; i < numFiles; i++) {
        fs_file_t file;
        int ret = fs_open(&file, testFiles[i], FS_O_CREATE | FS_O_WRITE);
        if (ret >= 0) {
            fs_close(&file);
        }
    }
    
    uint32_t create_endMs = k_uptime_get_32();
#ifdef CONFIG_TIMING_FUNCTIONS
    endTime = timing_counter_get();
    uint64_t createCycles = timing_cycles_get(&startTime, &endTime);
    timing_stop();
#endif
    
    // File deletion benchmark
#ifdef CONFIG_TIMING_FUNCTIONS
    timing_start();
    startTime = timing_counter_get();
#endif
    uint32_t deleteStartMs = k_uptime_get_32();
    
    for (size_t i = 0; i < numFiles; i++) {
        fs_unlink(testFiles[i]);
    }
    
    uint32_t deleteEndMs = k_uptime_get_32();
#ifdef CONFIG_TIMING_FUNCTIONS
    endTime = timing_counter_get();
    uint64_t deleteCycles = timing_cycles_get(&startTime, &endTime);
    timing_stop();
#endif
    
    uint32_t createMs = create_endMs - createStartMs;
    uint32_t deleteMs = deleteEndMs - deleteStartMs;
    
    LOG_INF("File creation time: %u ms", createMs);
    LOG_INF("Average per file: %.2f ms", (double)createMs / numFiles);
    LOG_INF("File deletion time: %u ms", deleteMs);
    LOG_INF("Average per file: %.2f ms", (double)deleteMs / numFiles);

#ifdef CONFIG_TIMING_FUNCTIONS
    uint64_t create_ns = timing_cycles_to_ns(createCycles);
    uint64_t delete_ns = timing_cycles_to_ns(deleteCycles);
    
    LOG_INF("Precise timing:");
    LOG_INF("  File creation time: %llu ns (%.2f ms)", create_ns, create_ns / 1000000.0);
    LOG_INF("  Average per file: %.2f ms", (create_ns / 1000000.0) / numFiles);
    LOG_INF("  File deletion time: %llu ns (%.2f ms)", delete_ns, delete_ns / 1000000.0);
    LOG_INF("  Average per file: %.2f ms", (delete_ns / 1000000.0) / numFiles);
#endif
    LOG_INF("");
}

int main() {
    LOG_INF("LittleFS and CDataLogger Benchmark Starting...");
    LOG_INF("Number of iterations: %zu", NUM_ITERATIONS);
    LOG_INF("");
    
    // // Wait for filesystem to be ready
    k_msleep(100);

    // Print initial filesystem stats
    printFilesystemStats("/lfs");
    LOG_INF("");

    // Benchmark different packet sizes with CDataLogger
    benchmarkDataloggerMode<SmallPacket>("CDataLogger Small Packet (Growing Mode)",
                                          "/lfs/small_growing.bin", LogMode::Growing);
    
    
    benchmarkDataloggerMode<MediumPacket>("CDataLogger Medium Packet (Growing Mode)",
                                           "/lfs/medium_growing.bin", LogMode::Growing);

    benchmarkDataloggerMode<LargePacket>("CDataLogger Large Packet (Growing Mode)",
                                          "/lfs/large_growing.bin", LogMode::Growing);

    // Benchmark different modes
    benchmarkDataloggerMode<MediumPacket>("CDataLogger Medium Packet (Circular Mode)",
                                           "/lfs/medium_circular.bin", LogMode::Circular, 500);

    benchmarkDataloggerMode<MediumPacket>("CDataLogger Medium Packet (FixedSize Mode)",
                                           "/lfs/medium_fixed.bin", LogMode::FixedSize, 500);

    // Raw filesystem benchmarks
    // benchmarkRawFilesystem();
    // benchmarkFileOperations();

    // Final filesystem stats
    printFilesystemStats("/lfs");

    LOG_INF("Benchmark completed!");
    return 0;
}