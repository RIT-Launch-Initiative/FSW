#include <zephyr/kernel.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "radio_module_functionality.h"

#define NUM_SOCKETS 4
#define SLEEP_TIME_MS   100
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

LOG_MODULE_REGISTER(main, CONFIG_APP_RADIO_MODULE_LOG_LEVEL);

// Queues
K_QUEUE_DEFINE(lora_tx_queue);
K_QUEUE_DEFINE(net_tx_queue);

// Devices
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct device *const lora_dev = DEVICE_DT_GET_ONE(semtech_sx1276);
static const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

// Networking
static int udp_sockets[NUM_SOCKETS] = {0};
static uint16_t udp_socket_ports[NUM_SOCKETS] = {LAUNCH_EVENT_NOTIFICATION_PORT,
                                 POWER_MODULE_BASE_PORT + POWER_MODULE_INA_DATA_PORT,
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT,
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT,
                                 };
l_udp_socket_list_t udp_socket_list = {
        .sockets = udp_sockets,
        .ports = udp_socket_ports,
        .num_sockets = NUM_SOCKETS
};


static int init_networking() {
    k_queue_init(&net_tx_queue);

    char ip[MAX_IP_ADDRESS_STR_LEN];

    int ret = l_create_ip_str_default_net_id(ip, RADIO_MODULE_ID, 1);
    if (ret < 0) {
        LOG_ERR("Failed to create IP address string: %d", ret);
        return -1;
    }
  
    ret = l_init_udp_net_stack(RADIO_MODULE_IP_ADDR);
    if (ret != 0) {
        LOG_ERR("Failed to initialize UDP networking stack: %d", ret);
        return -3;
    }

    for (int i = 0; i < udp_socket_list.num_sockets; i++) {
        udp_socket_list.sockets[i] = l_init_udp_socket(RADIO_MODULE_IP_ADDR, udp_socket_list.ports[i]);
        if (udp_socket_list.sockets[i] < 0) {
            LOG_ERR("Failed to create UDP socket: %d", udp_socket_list.sockets[i]);
        }
    }

    return 0;
}

static int init() {
    if (l_check_device(lora_dev) == 0) {
        l_lora_configure(lora_dev, false);
        init_lora_unique();
    }


    if (l_check_device(wiznet) == 0) {
        init_networking();
        init_udp_unique(&udp_socket_list);
    }

    start_tasks();

    return 0;
}


int main() {
    LOG_DBG("Starting radio module!\n");
    if (init()) {
        return -1;
    }

    return main_unique();
}
