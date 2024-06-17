// Self Include
#include "potato.h"

// Launch Includes
#include <launch_core/os/fs.h>
#include <zephyr/fs/fs.h>
// Zephyr Includes
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOGGING_THREAD_STACK_SIZE 512
LOG_MODULE_REGISTER(data_logger);

// Message Queues
K_MSGQ_DEFINE(logging_queue, sizeof(logging_packet_t), 500, 1);

// Threads
static void logging_task(void*);
K_THREAD_DEFINE(data_log_thread, LOGGING_THREAD_STACK_SIZE, logging_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0,
                1000);

// Extern Variables
extern uint32_t boot_count;

#define MAX_FILE_LEN 16

static void logging_task(void*) {
    logging_packet_t log_packet = {0};

    // Use the boot count to create a new directory for the logs
    while (boot_count == -1) {
        k_msleep(1);
        continue;
    } // Block until we get a boot count
    static char dir_name[MAX_FILE_LEN] = {0};
    snprintf(dir_name, sizeof(dir_name), "/lfs/%02d");
    fs_mkdir(dir_name);

    static char dir_name[MAX_FILE_LEN] = {0};
    snprintf(dir_name, sizeof(dir_name), "/lfs/%02d/dat");

    // Create file for logging

    while (1) {
        if (k_msgq_get(&logging_queue, &log_packet, K_FOREVER)) continue;

        // TODO: Filesystem logging calls
        k_msleep(1);
    }
}

static void adc_logging_task(void*) {
    potato_adc_telemetry_t log_packet = {0};

    // Use the boot count to create a new directory for the logs
    while (boot_count == -1) {
        k_msleep(1);
        continue;
    } // Block until we get a boot count
    // block a little while longer bc the other logging task makes the directory
    k_msleep(500);
    // Create file for logging

    while (1) {
        if (k_msgq_get(&logging_queue, &log_packet, K_FOREVER)) continue;

        // TODO: Filesystem logging calls
    }
}

void bin_telemetry_file() {
    static uint8_t file_count = 0;

    // Close current file (MIGHT NEED TO LOCK LOGGING THREAD)

    // Create new file and update global pointer for logging thread
    file_count++;

    // Log the file count and the frequency in another file for reference
}