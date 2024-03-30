#ifdef CONFIG_RADIO_MODULE_RECEIVER

// Self Include
#include "radio_module_functionality.h"

// Launch Includes
#include <launch_core/net/lora.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>

// Queues - Name, Entry Size, Queue Size, Alignment
K_MSGQ_DEFINE(udp_broadcast_queue, sizeof(l_lora_packet_t), 8, 1);

static void udp_broadcast_task(void *unused0, void *unused1, void *unused2) {
    l_lora_packet_t lora_packet = {0};

    while (true) {
        if (k_msgq_get(&udp_broadcast_queue, &lora_packet, K_FOREVER)) {
            // TODO: Get a socket
            l_send_udp_broadcast(0, lora_packet.payload, lora_packet.payload_len, lora_packet.port);
        }
    }
}

int init_lora_unique(const struct device *const lora_dev) {
    return lora_recv_async(lora_dev, 0); // TODO: Write callback function
}

int init_udp_unique(l_udp_socket_list_t *udp_socket_list) {
    return 0;
}

int start_tasks() {
    return 0;
}

int main_unique() {


    return 0;
}

#endif
