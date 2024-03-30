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

LOG_MODULE_REGISTER(radio_module_rxer);

// Queues - Name, Entry Size, Queue Size, Alignment
K_MSGQ_DEFINE(udp_broadcast_queue, sizeof(l_lora_packet_t), 8, 1);

// Threads
#define THREAD_STACK_SIZE 1024
static K_THREAD_STACK_DEFINE(udp_broadcast_stack, THREAD_STACK_SIZE);
static struct k_thread udp_broadcast_thread;

static void udp_broadcast_task(void *socket, void *unused1, void *unused2) {
    int sock = POINTER_TO_INT(socket);
    l_lora_packet_t lora_packet = {0};

    while (true) {
        if (k_msgq_get(&udp_broadcast_queue, &lora_packet, K_FOREVER)) {
            l_send_udp_broadcast(sock, lora_packet.payload, lora_packet.payload_len, lora_packet.port);
        }
    }
}

static void receiver_cb(const struct device *lora_dev, uint8_t *payload, uint16_t len, int16_t rssi, int8_t snr)
{
    ARG_UNUSED(lora_dev);
	ARG_UNUSED(len);

    // TODO: Determine how long copying l_lora_packet_t takes and if its too long for a callback. Maybe do zbus soon
    if (k_msgq_put(&udp_broadcast_queue, (l_lora_packet_t *) payload, K_MSEC(5)) < 0) {
        LOG_ERR("Failed to put received telemetry on the queue");
    }

    // TODO: Put LoRa stats on a queue too?
}

int init_lora_unique(const struct device *const lora_dev) {
    return lora_recv_async(lora_dev, &receiver_cb);
}

int init_udp_unique(l_udp_socket_list_t *udp_socket_list) {
    ARG_UNUSED(*udp_socket_list);

    int sock = l_init_udp_socket(RADIO_MODULE_IP_ADDR, RADIO_MODULE_BASE_PORT);
    if (sock < 0) {
        LOG_ERR("Failed to create socket for UDP broadcasts. Not sending received LoRa packets");
        return sock;
    }

    k_thread_create(&udp_broadcast_thread, &udp_broadcast_stack[0], THREAD_STACK_SIZE,
                    udp_broadcast_task, INT_TO_POINTER(sock), NULL, NULL,
                    K_PRIO_PREEMPT(5),
                    0,
                    K_NO_WAIT);
    k_thread_start(&udp_broadcast_thread);

    return 0;
}

int start_tasks() {
    return 0;
}

int main_unique() {


    return 0;
}

#endif
