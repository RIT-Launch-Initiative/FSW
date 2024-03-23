#include "zephyr/kernel/thread.h"
#if !defined(RADIO_MODULE_RECEIVER)

#include "radio_module_functionality.h"

#include <launch_core/dev/gnss.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(radio_module_txer);

// Callbacks
//GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), l_gnss_data_debug_cb);
//GNSS_SATELLITES_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), l_gnss_debug_sat_count_cb);

// Networking
#define UDP_RX_STACK_SIZE 1024
#define UDP_RX_BUFF_LEN 256 // TODO: Make this a KConfig
static uint8_t udp_rx_buffer[UDP_RX_BUFF_LEN];

#define LORA_TX_STACK_SIZE 1024

// Queues
static struct k_msgq lora_tx_queue;

// Threads
static K_THREAD_STACK_DEFINE(udp_rx_stack, UDP_RX_STACK_SIZE);
static struct k_thread udp_rx_thread;

static K_THREAD_STACK_DEFINE(lora_tx_stack, LORA_TX_STACK_SIZE);
static struct k_thread lora_tx_thread;

static void udp_rx_task(void *socks, void *buff_ptr, void *buff_len) {
    l_udp_socket_list_t *sock_list = (l_udp_socket_list_t*) socks;
    size_t len = POINTER_TO_INT(buff_len);
    
    while (true) {
        for (int i = 0; i < sock_list->num_sockets; i++) {
            // looping through sockets
            l_lora_packet_t packet = {0};
            packet.port = sock_list->ports[i];
            packet.payload_len = l_receive_udp(sock_list->sockets[i], packet.payload, len);

            if (unlikely(packet.payload_len < 0)) {
              LOG_ERR("Failed to receive UDP data (%d)", packet.payload_len);
              continue;
            }

            // this copies the packet to the queue
            k_msgq_put(&lora_tx_queue, &packet, K_NO_WAIT);
        }
    }
}

static void lora_tx_task() {
    const struct device *const lora_dev = DEVICE_DT_GET_ONE(semtech_sx1276);

    while (1) {
        // pop from queue
        l_lora_packet_t packet = {0};
        k_msgq_get(&lora_tx_queue, &packet, K_FOREVER);
        l_lora_tx(lora_dev, (uint8_t*)&packet, packet.payload_len + sizeof(packet.port));
    }


}

int init_lora_unique(const struct device *const lora_dev) {
    return l_lora_set_tx_rx(lora_dev, true);
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
    k_thread_create(&lora_tx_thread, &lora_tx_stack[0], 
                    LORA_TX_STACK_SIZE,
                    lora_tx_task, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(5),
                    0,
                    K_NO_WAIT);
    k_thread_start(&lora_tx_thread);
    return 0;
}

int main_unique() {
    const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
    const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

    while (true) {
        gpio_pin_toggle_dt(&led0);
        gpio_pin_toggle_dt(&led1);
        k_msleep(100);
    }

    return 0;
}

#endif