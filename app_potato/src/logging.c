// Self Include
#include "potato.h"

// Launch Includes

// Zephyr Includes
#include <zephyr/kernel.h>

K_MSGQ_DEFINE(logging_queue, sizeof(logging_packet_t), 500, 1);


static void logging_task(void) {
    logging_packet_t log_packet = {0};
    while (1) {
        if (k_msgq_get(&logging_queue, &log_packet, K_FOREVER)) continue;

        // TODO: Filesystem logging calls
    }
}