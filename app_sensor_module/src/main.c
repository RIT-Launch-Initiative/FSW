#include <app_version.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/fs/fs.h>
//#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/storage/flash_map.h>

#include "sensors.h"
#include <launch_core/net_utils.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
K_QUEUE_DEFINE(net_tx_queue);

#define STACK_SIZE (512)
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

static void init(void) {
    // Queues
    k_queue_init(&net_tx_queue);

    const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);
    if (!init_eth_iface(wiznet)) {
        printk("Ethernet ready\n");
        if (!init_net_stack()) {
            printk("Network stack initialized\n");
        }
    }

}

int main() {
    // init();

    uint8_t data;
    while (1) {
        send_udp_broadcast(&data, 1, 69);
   } 

    return 0;
}


