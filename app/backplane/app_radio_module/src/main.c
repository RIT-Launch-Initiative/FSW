// Application Includes
#include "radio_module_functionality.h"

#include <launch_core_classic/os/fs.h>
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
static const struct device* const lora_dev = DEVICE_DT_GET_ONE(semtech_sx1276);
static const struct device* const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

static int init_networking() {
    int ret = l_init_udp_net_stack_by_device(wiznet, RADIO_MODULE_IP_ADDR);
    if (ret != 0) {
        LOG_ERR("Failed to initialize UDP networking stack: %d", ret);
        return ret;
    } else {
        LOG_INF("Initialized networking stack");
    }

    return 0;
}

int main() {
    LOG_DBG("Starting radio module!\n");

    const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
    const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
    gpio_pin_set_dt(&led0, 1);
    gpio_pin_set_dt(&led1, 1);

    if (l_check_device(lora_dev) == 0) {
        if (l_lora_set_frequency(lora_dev, 905000000)) {
            LOG_ERR("Failed to properly set LoRa frequency");
        }

        if (l_lora_set_bandwidth(lora_dev, BW_125_KHZ)) {
            LOG_ERR("Failed to properly set LoRa bandwidth");
        }

        if (l_lora_set_data_rate(lora_dev, SF_12)) {
            LOG_ERR("Failed to set LoRa data rate");
        }

        if (l_lora_set_spreading_factor(lora_dev, CR_4_6)) {
            LOG_ERR("Failed to set LoRa coding rate");
        }

        init_lora_unique(lora_dev);
    }

    if (l_check_device(wiznet) == 0) {
        init_networking();
        init_udp_unique();
    }

    return main_unique();
}
