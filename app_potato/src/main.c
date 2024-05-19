
#include <launch_core/os/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_potato);

// Queues
static K_QUEUE_DEFINE(slip_tx_queue);

// Global Variables
bool logging_enabled = false;
uint32_t boot_count = -1;




int main() {
    boot_count = l_fs_boot_count_check();

    return 0;
}
