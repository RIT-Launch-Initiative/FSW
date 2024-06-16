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
K_MSGQ_DEFINE(rx_telem_queue, sizeof(l_lora_packet_t), 8, 1);
K_MSGQ_DEFINE(statistics_queue, sizeof(l_lora_packet_t), 8, 1);

// Threads
#define THREAD_STACK_SIZE 1024
static K_THREAD_STACK_DEFINE(udp_broadcast_stack, THREAD_STACK_SIZE);
static struct k_thread udp_broadcast_thread;

static void udp_broadcast_task(void *socket, void *unused1, void *unused2) {
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);

    int sock = POINTER_TO_INT(socket);
    l_lora_packet_t lora_packet = {0};
    l_lora_statistics_t lora_statistics = {0};

    while (true) {
        if (k_msgq_get(&rx_telem_queue, &lora_packet, K_MSEC(100))) {
            l_send_udp_broadcast(sock, lora_packet.payload, lora_packet.payload_len, lora_packet.port);
        }

        if (k_msgq_get(&statistics_queue, &lora_statistics, K_MSEC(100))) {
            l_send_udp_broadcast(sock, (uint8_t *) &lora_statistics, sizeof(l_lora_statistics_t),
                                 RADIO_MODULE_BASE_PORT);
        }
    }
}

static void receiver_cb(const struct device *lora_dev, uint8_t *payload, uint16_t len, int16_t rssi, int8_t snr) {
    ARG_UNUSED(lora_dev);
    ARG_UNUSED(len);

    static l_lora_statistics_t statistics = {0};
    statistics.count++;
    statistics.rssi = rssi;
    statistics.snr = snr;

    // TODO: Determine how long copying l_lora_packet_t takes and if its too long for a callback. Maybe do zbus soon
    if (k_msgq_put(&rx_telem_queue, payload, K_MSEC(5)) < 0) {
        LOG_ERR("Failed to queue received telemetry");
    }

    if (k_msgq_put(&statistics_queue, &statistics, K_MSEC(5)) < 0) {
        LOG_ERR("Failed to queue statistics");
    }

    LOG_INF("Received %d bytes. RSSI: %d SNR: %d", len, rssi, snr);
}

int init_lora_unique(const struct device *const lora_dev) { return lora_recv_async(lora_dev, &receiver_cb); }

int init_udp_unique() {
    int sock = l_init_udp_socket(RADIO_MODULE_IP_ADDR, RADIO_MODULE_BASE_PORT);
    if (sock < 0) {
        LOG_ERR("Failed to create socket for UDP broadcasts. Not sending received LoRa packets");
        return sock;
    }

    k_thread_create(&udp_broadcast_thread, &udp_broadcast_stack[0], THREAD_STACK_SIZE, udp_broadcast_task,
                    INT_TO_POINTER(sock), NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
    k_thread_start(&udp_broadcast_thread);

    return 0;
}

int main_unique() {
    LOG_INF("Started radio module RECEIVER");
    return 0;
}

#endif
