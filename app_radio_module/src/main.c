#include <launch_core/backplane_defs.h>
#include <launch_core/device_utils.h>
#include <launch_core/lora_utils.h>
#include <launch_core/net_utils.h>
#include <zephyr/console/console.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>

#include "ubxlib_utils.h"

#define SLEEP_TIME_MS 100

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

gnss_dev_t *gnss_dev;
extern int start_maxm10s(gnss_dev_t *dev);

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);
K_QUEUE_DEFINE(lora_tx_queue);
K_QUEUE_DEFINE(net_tx_queue);

// GNSS init thread
static void gnss_init_task(void) {
    printk("Initializing GNSS...\n");
    int ret = start_maxm10s(gnss_dev);
    if (!ret)
        printk("GNSS initialized\n");
    else
        printk("Error initializing GNSS. Got %d", ret);
}
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct device *const lora_dev = DEVICE_DT_GET_ONE(semtech_sx1276);
static const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

#define GNSS_INIT_STACK_SIZE 2 << 10
#define GNSS_INIT_PRIORITY 4
K_THREAD_STACK_DEFINE(gnss_init_stack_area, GNSS_INIT_STACK_SIZE);
static struct k_thread gnss_init_thread_data;
static k_tid_t gnss_init_tid;

// init method
static void init() {
    char ip[MAX_IP_ADDRESS_STR_LEN];
    int ret = -1;

    k_queue_init(&net_tx_queue);

    if (!l_check_device(lora_dev)) {
        l_lora_configure(lora_dev, false);
    }

    if (0 > l_create_ip_str_default_net_id(ip, RADIO_MODULE_ID, 1)) {
        LOG_ERR("Failed to create IP address string: %d", ret);
        return -1;
    }

    if (!l_check_device(wiznet)) {
        l_init_udp_net_stack("192.168.1.1");
    }

    // start gnss init thread
    gnss_init_tid = k_thread_create(&gnss_init_thread_data, gnss_init_stack_area,
                                    K_THREAD_STACK_SIZEOF(gnss_init_stack_area),
                                    gnss_init_task, NULL, NULL, NULL,
                                    GNSS_INIT_PRIORITY, 0, K_NO_WAIT);
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
