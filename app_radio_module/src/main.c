#include <zephyr/kernel.h>

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <launch_core/backplane_defs.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/net/lora.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>

#define SLEEP_TIME_MS   100
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

LOG_MODULE_REGISTER(main);
K_QUEUE_DEFINE(lora_tx_queue);
K_QUEUE_DEFINE(net_tx_queue);

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct device *const lora_dev = DEVICE_DT_GET_ONE(semtech_sx1276);
static const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data) {
    if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
        LOG_INF("%s has fix!\r\n", dev->name);
    } else {
        LOG_INF("%s has no fix!\r\n", dev->name);
    }
}

GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnss_data_cb);

#if CONFIG_GNSS_SATELLITES
static void gnss_satellites_cb(const struct device *dev, const struct gnss_satellite *satellites, uint16_t size) {
    LOG_INF("%s reported %u satellites!\r\n", dev->name, size);
}
#endif
GNSS_SATELLITES_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnss_satellites_cb);

static int init() {
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

    return 0;
}



int main() {
    LOG_DBG("Starting radio module!\n");
    if (init()) {
        return -1;
    }

//    while (1) {
//        gpio_pin_toggle_dt(&led0);
//        gpio_pin_toggle_dt(&led1);
//        k_msleep(100);
//    }

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
