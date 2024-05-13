// Application Includes
#include "radio_module_functionality.h"

#include <launch_core/os/fs.h>
// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

#define SLEEP_TIME_MS 100

LOG_MODULE_REGISTER(main, CONFIG_APP_RADIO_MODULE_LOG_LEVEL);

// Queues
K_QUEUE_DEFINE(net_tx_queue);

// Devices
static const struct device *const lora_dev = DEVICE_DT_GET_ONE(semtech_sx1276);
static const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

static int init_networking() {
    int ret = l_init_udp_net_stack_by_device(wiznet, RADIO_MODULE_IP_ADDR);
    if (ret != 0) {
        LOG_ERR("Failed to initialize UDP networking stack: %d", ret);
        return ret;
    }

    return 0;
}

int main() {
    LOG_DBG("Starting radio module!\n");

    if (l_check_device(lora_dev) == 0) {
        init_lora_unique(lora_dev);
    }

    if (l_check_device(wiznet) == 0) {
        init_networking();
        init_udp_unique();
    }

    start_tasks();

    return main_unique();
}
