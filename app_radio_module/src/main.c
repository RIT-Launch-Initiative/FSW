#include <zephyr/kernel.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <launch_core/backplane_defs.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/dev/gnss.h>
#include <launch_core/net/lora.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>

#define SLEEP_TIME_MS   100
#define UDP_RX_STACK_SIZE 1024
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

LOG_MODULE_REGISTER(main, CONFIG_APP_RADIO_MODULE_LOG_LEVEL);

// Queues
K_QUEUE_DEFINE(lora_tx_queue);
K_QUEUE_DEFINE(net_tx_queue);

// Threads
static K_THREAD_STACK_DEFINE(udp_rx_stack, UDP_RX_STACK_SIZE);
static struct k_thread udp_rx_thread;

// Callbacks
GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), l_gnss_data_debug_cb);
GNSS_SATELLITES_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), l_gnss_debug_sat_count_cb);

// Devices
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct device *const lora_dev = DEVICE_DT_GET_ONE(semtech_sx1276);
static const struct device *const wiznet = DEVICE_DT_GET_ONE(wiznet_w5500);

// Networking
#define UDP_RX_BUFF_LEN 256 // TODO: Make this a KConfig
#define NUM_SOCKETS 2
static uint8_t udp_rx_buffer[UDP_RX_BUFF_LEN];
static int udp_sockets[NUM_SOCKETS] = {0};
static int udp_socket_ports[NUM_SOCKETS] = {10000};
static l_udp_socket_list_t udp_socket_list = {
        .sockets = udp_sockets,
        .num_sockets = 2
};

static void udp_rx_task(void *socks, void *buff_ptr, void *buff_len) {
    l_default_receive_thread(socks, buff_ptr, buff_len);
}

static void init_networking() {
    if (l_check_device(wiznet) != 0) {
        LOG_ERR("Wiznet device not found");
        return;
    }

    int ret = l_init_udp_net_stack_default(RADIO_MODULE_IP_ADDR);
    if (ret != 0) {
        LOG_ERR("Failed to initialize UDP networking stack: %d", ret);
        return;
    }

    for (int i = 0; i < udp_socket_list.num_sockets; i++) {
        udp_socket_list.sockets[i] = l_init_udp_socket(RADIO_MODULE_IP_ADDR, udp_socket_ports[i]);
        if (udp_socket_list.sockets[i] < 0) {
            LOG_ERR("Failed to create UDP socket: %d", udp_socket_list.sockets[i]);
            return;
        }
    }

    k_thread_create(&udp_rx_thread, &udp_rx_stack[0], UDP_RX_STACK_SIZE,
                    udp_rx_task, &udp_socket_list, udp_rx_buffer, INT_TO_POINTER(UDP_RX_BUFF_LEN),
                    K_PRIO_PREEMPT(5),
                    0,
                    K_NO_WAIT);
    k_thread_start(&udp_rx_thread);

}

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

    init_networking();


    return 0;
}



int main() {
    LOG_DBG("Starting radio module!\n");
    if (init()) {
        return -1;
    }

    while (1) {
        gpio_pin_toggle_dt(&led0);
        gpio_pin_toggle_dt(&led1);
        l_send_udp_broadcast(udp_sockets[0], (uint8_t *) "Launch!", 7, 10000);
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
