#include "zephyr/logging/log.h"

#include <f_core/os/c_raw_datalogger.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(main);

struct TestData {
    const char name[16];
    int a;
    int b;
    int c;
    int d;
};

int main() {
    const device* flash = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
    if (!device_is_ready(flash)) {
        LOG_ERR("Flash device not ready!");
        return -1;
    } else {
        // Print flash size
        uint64_t flashSize = 0;
        int ret = flash_get_size(flash, &flashSize);
        if (ret < 0) {
            LOG_ERR("Failed to get flash size: %d", ret);
            return -1;
        }

        LOG_INF("Flash Size: %llu bytes", flashSize);
    }
    off_t start_addr = 0x00000000;
    size_t erase_size = 0x00100000; // 1 MiB

    int err = flash_erase(flash, start_addr, erase_size);
    if (err < 0) {
        LOG_ERR("Failed to erase flash: %d", err);
        return -1;
    }

    // Fixed
    off_t nextAddr = 0x00000000;
    const int packetsInFixedFile = 5;
    const size_t fixedFileSize = sizeof(TestData) * packetsInFixedFile + sizeof(DataloggerMetadata);
    CRawDataLogger<TestData, 3> fixedLogger(flash, nextAddr, fixedFileSize, "test_fixed", DataloggerMode::Fixed);
    for (int i = 0; i < packetsInFixedFile; ++i) {
        TestData data = { "fixed", i, i + 1, i + 2, i + 3 };
        int ret = fixedLogger.Write(data);
        if (ret < 0) {
            LOG_ERR("Error writing data for fixed: %d", ret);
        } else {
            LOG_INF("Successfully wrote %d entries to fixed logger", i + 1);
        }
    }
    LOG_INF("Finished writing data for fixed logger.");
    if (fixedLogger.Write(TestData{ "fixed_overflow", 0, 1, 2, 3 }) != -ENOSPC) {
        LOG_ERR("Error: Should not have been able to write more data to fixed logger");
    }

    nextAddr += fixedFileSize;

    // LinkedFixed
    const size_t linkedFixedFileSize = (sizeof(TestData) * 5) + sizeof(DataloggerMetadata);
    CRawDataLogger<TestData, 3> linkedFixedLogger(flash, nextAddr, linkedFixedFileSize, "test_linked_fixed", DataloggerMode::LinkedFixed);
    for (int i = 0; i < 5; ++i) {
        TestData data = { "linked_fixed", i + 1, i + 1, i + 2, i + 3 };
        int ret = linkedFixedLogger.Write(data);
        if (ret < 0) {
            LOG_ERR("Error writing data for linked fixed: %d", ret);
        } else {
            LOG_INF("Successfully wrote %d entries to linked fix logger", i + 1);
        }
    }

    LOG_INF("Finished writing data for linked fixed logger.");


    nextAddr += linkedFixedFileSize;
    // Add a file in between to test finding next linked space using a fixed logger
    const size_t intermediateFileSize = sizeof(TestData) + sizeof(DataloggerMetadata);
    CRawDataLogger<TestData, 1> intermediateLogger(flash, nextAddr, intermediateFileSize, "intermediate", DataloggerMode::Fixed);
    TestData intermediateData = { "intermediate", 0, 1, 2, 3 };
    intermediateLogger.Write(intermediateData);
    if (intermediateLogger.GetLastError() != 0) {
        LOG_ERR("Error writing intermediate data: %d", intermediateLogger.GetLastError());
    }

    LOG_INF("Finished writing data for intermediate fixed logger");

    nextAddr += intermediateFileSize;
    // LinkedFixed should skip the intermediate file
    for (int i = 0; i < 10; ++i) {
        TestData data = { "linked_fixed2", i, i + 1, i + 2, i + 3 };
        int ret = linkedFixedLogger.Write(data);
        if (ret < 0) {
            LOG_ERR("Error writing data for linked fixed 2: %d", ret);
        } else {
            LOG_INF("Successfully wrote %d entries to linked fixed logger", i + 1);
        }
    }

    LOG_INF("Finished writing more data for linked fixed logger.");

    // Test LinkedTruncate
    CRawDataLogger<TestData, 3> linkedTruncateLogger(flash, nextAddr, linkedFixedFileSize, "test_linked_truncate", DataloggerMode::LinkedTruncate);

    nextAddr += linkedFixedFileSize - sizeof(DataloggerMetadata) + sizeof(TestData) * 2;
    CRawDataLogger<TestData, 1> anotherIntermediateLogger(flash, nextAddr, intermediateFileSize, "another_intermediate", DataloggerMode::Fixed);
    anotherIntermediateLogger.Write(intermediateData);

    if (anotherIntermediateLogger.GetLastError() != 0) {
        LOG_ERR("Error writing another intermediate data: %d", anotherIntermediateLogger.GetLastError());
    }
    LOG_INF("Finished writing data for another intermediate fixed logger");


    for (int i = 0; i < 10; ++i) {
        TestData data = { "linked_truncate", i, i + 1, i + 2, i + 3 };
        int ret = linkedTruncateLogger.Write(data);
        if (ret < 0) {
            LOG_ERR("Error writing data for linked truncate: %d", ret);
        } else {
            LOG_INF("Successfully wrote %d entries to linked truncate logger", i + 1);
        }
    }

    LOG_INF("Finished writing data for linked truncate logger");

    return 0;
}
