// Self Include
#include "potato.h"

// Launch Includes

// Zephyr Includes
#include <zephyr/kernel.h>

#define LOGGING_THREAD_STACK_SIZE 512

// Message Queues
K_MSGQ_DEFINE(logging_queue, sizeof(logging_packet_t), 500, 1);

// Threads
static void logging_task(void*);
K_THREAD_DEFINE(data_log_thread, LOGGING_THREAD_STACK_SIZE, logging_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0,
                1000);

// Extern Variables
extern uint32_t boot_count;

static void logging_task(void*) {
    logging_packet_t log_packet = {0};

    // Use the boot count to create a new directory for the logs

    // Create file for logging

    while (1) {
        if (k_msgq_get(&logging_queue, &log_packet, K_FOREVER)) continue;

        // TODO: Filesystem logging calls
    }
}

void bin_telemetry_file() {
    static uint8_t file_count = 0;

    // Close current file (MIGHT NEED TO LOCK LOGGING THREAD)

    // Create new file
    file_count++;

    // Log the file count and the frequency in another file for reference

}