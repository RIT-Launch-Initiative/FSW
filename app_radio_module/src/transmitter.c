#if !defined(RADIO_MODULE_RECEIVER)

#include <launch_core/dev/gnss.h>
#include <zephyr/drivers/gpio.h>
#include "radio_module_functionality.h"

// Callbacks
GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), l_gnss_data_debug_cb);
GNSS_SATELLITES_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), l_gnss_debug_sat_count_cb);

// Networking
#define UDP_RX_STACK_SIZE 1024
#define UDP_RX_BUFF_LEN 256 // TODO: Make this a KConfig
static uint8_t udp_rx_buffer[UDP_RX_BUFF_LEN];

// Threads
static K_THREAD_STACK_DEFINE(udp_rx_stack, UDP_RX_STACK_SIZE);
static struct k_thread udp_rx_thread;

static void udp_rx_task(void *socks, void *buff_ptr, void *buff_len) {
    l_default_receive_thread(socks, buff_ptr, buff_len);
}


int init_lora_unique(const struct device *const lora_dev) {
    return l_lora_configure(lora_dev, true);
}

int init_udp_unique(l_udp_socket_list_t *udp_socket_list) {
    k_thread_create(&udp_rx_thread, &udp_rx_stack[0], UDP_RX_STACK_SIZE,
                    udp_rx_task, udp_socket_list, udp_rx_buffer, INT_TO_POINTER(UDP_RX_BUFF_LEN),
                    K_PRIO_PREEMPT(5),
                    0,
                    K_NO_WAIT);
    k_thread_start(&udp_rx_thread);

    return 0;
}

int start_tasks() {


    return 0;
}

int main_unique() {
    return 0;

}

#endif
