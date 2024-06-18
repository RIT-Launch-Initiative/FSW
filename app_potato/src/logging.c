// Self Include
#include "potato.h"

// Launch Includes
#include <launch_core/os/fs.h>
#include <zephyr/fs/fs.h>
// Zephyr Includes
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOGGING_THREAD_STACK_SIZE 1024
LOG_MODULE_REGISTER(data_logger);

// Message Queues

K_MSGQ_DEFINE(logging_queue, sizeof(potato_telemetry_t), 500, 1);
K_MSGQ_DEFINE(adc_logging_queue, sizeof(potato_adc_telemetry_t), 500, 1);

// Threads
static void logging_task(void*);
K_THREAD_DEFINE(data_log_thread, LOGGING_THREAD_STACK_SIZE, logging_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0,
                1000);
static void adc_logging_task(void*);
K_THREAD_DEFINE(adc_log_thread, LOGGING_THREAD_STACK_SIZE, adc_logging_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0,
                1000);

// Extern Variables
extern uint32_t boot_count;
extern bool logging_enabled;

#define MAX_FILE_LEN 16

#define DATA_SAMPLE_COUNT 20000
#define ADC_SAMPLE_COUNT  20000

static void logging_task(void*) {
    potato_telemetry_t packet = {0};
    int err = 0;

    // Block until we get a boot count
    SPIN_WHILE(boot_count == -1, 1);

    // Use the boot count to create a new directory for the logs
    static char dir_name[MAX_FILE_LEN] = {0};
    snprintf(dir_name, sizeof(dir_name), "/lfs/%02d", boot_count);
    fs_mkdir(dir_name);

    static char fil_name[MAX_FILE_LEN] = {0};
    snprintf(fil_name, sizeof(fil_name), "/lfs/%02d/dat", boot_count);

    l_fs_file_t fil_file = {
        .fname = fil_name,
        .width = sizeof(potato_telemetry_t),
        .mode = SLOG_ONCE,
        .size = sizeof(potato_telemetry_t) * DATA_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };
    l_fs_init(&fil_file);

    SPIN_WHILE(!logging_enabled, 1);

    LOG_INF("Accepting Sensor Data");
    bool out_of_space = false;

    // Create file for logging
    while (logging_enabled && !out_of_space) {
        if (0 == k_msgq_get(&logging_queue, &packet, K_SECONDS(1))) {
            l_fs_write(&fil_file, (uint8_t*) &packet, &err);
            out_of_space = err == -ENOSPC;
        }
    }
    LOG_INF("Stop Accepting Sensor Data");
    // Flush any remaining messages to file
    while (0 == k_msgq_get(&logging_queue, &packet, K_NO_WAIT) && !out_of_space) {
        l_fs_write(&fil_file, (uint8_t*) &packet, &err);
        out_of_space = err == -ENOSPC;
    }

    l_fs_close(&fil_file);
    LOG_INF("Sensor Data File Closed");
}

static void adc_logging_task(void*) {
    potato_adc_telemetry_t packet = {0};
    int err = 0;

    // block until we get a boot count
    SPIN_WHILE(boot_count == -1, 1)
    // block a little while longer bc the other logging task makes the directory
    k_msleep(500);

    static char fil_name[MAX_FILE_LEN] = {0};
    snprintf(fil_name, sizeof(fil_name), "/lfs/%02d/adc", boot_count);

    l_fs_file_t fil_file = {
        .fname = fil_name,
        .width = sizeof(potato_adc_telemetry_t),
        .mode = SLOG_ONCE,
        .size = sizeof(potato_adc_telemetry_t) * ADC_SAMPLE_COUNT,
        .initialized = false,
        .file = {0},
        .dirent = {0},
        .vfs = {0},
        .wpos = 0,
    };
    l_fs_init(&fil_file);

    LOG_INF("ADC Log Ready");

    // wait for flight to start saving this data
    SPIN_WHILE(!logging_enabled, 1)
    LOG_INF("Accepting ADC Data");
    // Create file for logging
    while (logging_enabled) {
        if (0 == k_msgq_get(&adc_logging_queue, &packet, K_SECONDS(1))) {
            l_fs_write(&fil_file, (uint8_t*) &packet, &err);
        }
    }
    LOG_INF("Stop Accepting ADC Data");
    // Flush any remaining messages to the file
    while (0 == k_msgq_get(&adc_logging_queue, &packet, K_NO_WAIT)) {
        l_fs_write(&fil_file, (uint8_t*) &packet, &err);
    }
    l_fs_close(&fil_file);
}

void bin_telemetry_file() {
    static uint8_t file_count = 0;

    // Close current file (MIGHT NEED TO LOCK LOGGING THREAD)

    // Create new file and update global pointer for logging thread
    file_count++;

    // Log the file count and the frequency in another file for reference
}