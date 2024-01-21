#include <zephyr/kernel.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <launch_core/lora_utils.h>
#include <launch_core/net_utils.h>
#include <launch_core/device_utils.h>

#define SLEEP_TIME_MS   100
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
K_QUEUE_DEFINE(lora_tx_queue);
K_QUEUE_DEFINE(net_tx_queue);

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct device *const lora_dev = DEVICE_DT_GET_ONE(semtech_sx1276);
static const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

static void init() {
    if (!l_check_device(lora_dev)) {
        l_lora_configure(lora_dev, false);
    }

    if (!l_check_device(wiznet)) {
        l_init_udp_net_stack("192.168.1.1");
    }
}

int main() {
    uint8_t tx_buff[255] = {0};
    uint8_t tx_buff_len = 0;

    LOG_DBG("Starting radio module!\n");
    init();

    while (1) {
        gpio_pin_toggle_dt(&led0);
        k_msleep(100);
    }

    return 0;
}


// int main() {
//     init();
//     printk("Receiver started\n");
//     while (1) {
//         int ret = lora_recv_async(lora_dev, lora_debug_recv_cb);
//     }
//
//     return 0;
// }
