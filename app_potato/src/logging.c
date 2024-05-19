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

static void logging_task(void*) {
    logging_packet_t log_packet = {0};
    while (1) {
        if (k_msgq_get(&logging_queue, &log_packet, K_FOREVER)) continue;

        // TODO: Filesystem logging calls
    }
}