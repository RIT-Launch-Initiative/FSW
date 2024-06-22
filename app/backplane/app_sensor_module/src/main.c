// Self Include
#include "sensor_module.h"

// Launch Includes
#include <launch_core_classic/backplane_defs.h>
#include <launch_core_classic/dev/dev_common.h>
#include <launch_core_classic/os/fs.h>
#include <launch_core_classic/utils/event_monitor.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_SENSOR_MODULE_LOG_LEVEL);

void init() {
    init_networking();
}

int main() {
    init();
    return 0;
}
