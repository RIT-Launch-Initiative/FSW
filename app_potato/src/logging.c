// Self Include
#include "potato.h"

// Launch Includes
#include <launch_core/os/fs.h>
#include <zephyr/fs/fs.h>
// Zephyr Includes
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOGGING_THREAD_STACK_SIZE 2048
LOG_MODULE_REGISTER(data_logger);

// Message Queues
#define ADC_LOG_FILE_SIZE (120000000 / sizeof(potato_adc_telemetry_t))
K_MSGQ_DEFINE(adc_logging_queue, sizeof(potato_adc_telemetry_t), 1000, 1);

// Threads
static void adc_logging_task(void*);
K_THREAD_DEFINE(adc_log_thread, LOGGING_THREAD_STACK_SIZE, adc_logging_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0,
                1000);

// Extern Variables
extern uint32_t boot_count;
extern bool logging_enabled;

#define MAX_FILE_LEN 16

#define DATA_SAMPLE_COUNT 20000
// assume no barom
#define ADC_SAMPLE_COUNT 18000000000000
#define ADC_SAMPLE_TIME 1 // 1 Hz to fill above in 5 hours

static void adc_logging_task(void*) {
    potato_adc_telemetry_t packet = {0};
    int err = 0;

    // block until we get a boot count
    SPIN_WHILE(boot_count == -1, 1)
    // Use the boot count to create a new directory for the logs
    static char dir_name[MAX_FILE_LEN] = {0};
    snprintf(dir_name, sizeof(dir_name), "/lfs/%02d", boot_count);
    fs_mkdir(dir_name);

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
    LOG_INF("Accepting ADC Data");
    // Create file for logging
    while (true) {
        if (0 == k_msgq_get(&adc_logging_queue, &packet, K_SECONDS(1))) {
            l_fs_write(&fil_file, (uint8_t*) &packet, &err);
            if (err == -ENOSPC) {
                break;
            }
        }
    }
    LOG_INF("Stop Accepting ADC Data");
    l_fs_close(&fil_file);
}
