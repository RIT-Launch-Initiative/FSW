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

    // Rotating
    off_t nextAddr = 0x00000000;
    const size_t rotatingFileSize = sizeof(TestData) * 5 + sizeof(DataloggerMetadata);
    CRawDataLogger<TestData, 3> logger(flash, nextAddr, rotatingFileSize, "test_rotating", DataloggerMode::Rotating);
    for (int i = 0; i < 10; ++i) {
        TestData data = { "rotating", i, i + 1, i + 2, i + 3 };
        int ret = logger.Write(data);
        if (ret < 0) {
            LOG_ERR("Error writing data for rotating: %d", ret);
        }
    }

    LOG_INF("Finished writing data for rotating logger");

    nextAddr += rotatingFileSize;

    // Fixed
    const size_t fixedFileSize = sizeof(TestData) * 5 + sizeof(DataloggerMetadata);
    CRawDataLogger<TestData, 3> fixedLogger(flash, nextAddr, fixedFileSize, "test_fixed", DataloggerMode::Fixed);
    for (int i = 0; i < 10; ++i) {
        TestData data = { "fixed", i, i + 1, i + 2, i + 3 };
        int ret = fixedLogger.Write(data);
        if (ret < 0) {
            LOG_ERR("Error writing data for fixed: %d", ret);
        }
    }
    LOG_INF("Finished writing data for fixed logger.");

    nextAddr += fixedFileSize;

    // LinkedFixed
    const size_t linkedFixedFileSize = sizeof(TestData) * 5 + sizeof(DataloggerMetadata);
    CRawDataLogger<TestData, 3> linkedFixedLogger(flash, nextAddr, linkedFixedFileSize, "test_linked_fixed", DataloggerMode::LinkedFixed);
    for (int i = 0; i < 10; ++i) {
        TestData data = { "linked_fixed", i, i + 1, i + 2, i + 3 };
        int ret = linkedFixedLogger.Write(data);
        if (ret < 0) {
            LOG_ERR("Error writing data for linked fixed: %d", ret);
        }
    }

    LOG_INF("Finished writing data for linked fixed logger.");


    nextAddr += linkedFixedFileSize;
    // Add a file in between to test finding next linked space using a fixed logger
    const size_t intermediateFileSize = sizeof(TestData) * 1 + sizeof(DataloggerMetadata);
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
        }
    }

    LOG_INF("Finished writing data for linked truncate logger");

    return 0;
}
