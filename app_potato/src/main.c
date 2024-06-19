// Self Include
#include "potato.h"

// Launch Includes
#include <launch_core/os/fs.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_potato);

// Queues
static K_QUEUE_DEFINE(slip_tx_queue);

// Global Variables
uint32_t boot_count = -1;
bool boost_detected = false;


int main() {
    boot_count = l_fs_boot_count_check();
    return 0;
}
