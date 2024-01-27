#include <app_version.h>

#include <launch_core/device_utils.h>
#include <launch_core/net_utils.h> // TODO: Might need for SLIP

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_POTATO_LOG_LEVEL);
K_QUEUE_DEFINE(net_tx_queue);

#define STACK_SIZE (512)

static int init(void) {
    return 0;
}

int main() {
    if (!init()) {
        return -1;
    }

    return 0;
}


