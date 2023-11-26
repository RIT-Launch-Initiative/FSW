#include <zephyr/kernel.h>
#include <app_version.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/console/console.h>

#include "net_utils.h"
#include "lora_utils.h"


#define SLEEP_TIME_MS   100

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

extern const struct device *const lora_dev;

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
K_QUEUE_DEFINE(lora_tx_queue);
K_QUEUE_DEFINE(net_tx_queue);

static void init() {
    k_queue_init(&lora_tx_queue);
    k_queue_init(&net_tx_queue);

    if (!init_sx1276(lora_dev)) {
        int ret = lora_configure(lora_dev, false);
        if (ret != 0) {
            printk("Error initializing LORA device. Got %d", ret);
        }
    }

    if (!init_eth_iface()) {
        init_net_stack();
    }
}


int main() {
    const struct device *uart_dev = DEVICE_DT_GET(DT_ALIAS(dbguart));
    
    uint8_t tx_buff[255] = {0};
    uint8_t tx_buff_len = 0;

    printk("Starting radio module!\n");
    init();
    
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
