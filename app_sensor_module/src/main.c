// Self Include
#include "sensor_module.h"

// Launch Includes
#include <launch_core/backplane_defs.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/os/fs.h>
#include <launch_core/utils/event_monitor.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/smf.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_SENSOR_MODULE_LOG_LEVEL);

void init() {
    init_networking();
}

int main() {
    l_fs_boot_count_check();

    return 0;
}
