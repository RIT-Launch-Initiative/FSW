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

// Forward Declares
static void udp_broadcast_task(void);

// Threads
#define THREAD_STACK_SIZE 1024
K_THREAD_DEFINE(udp_bcast_thread, THREAD_STACK_SIZE, udp_broadcast_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

static void udp_broadcast_task(void) {
    static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
    static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

    int sock = l_init_udp_socket(RADIO_MODULE_IP_ADDR, RADIO_MODULE_BASE_PORT);
    if (sock < 0) {
        LOG_ERR("Failed to create socket for UDP broadcasts. Not sending received LoRa packets");
        return;
    }

    l_lora_packet_t lora_packet = {0};
    l_lora_statistics_t lora_statistics = {0};

    while (true) {
        if (k_msgq_get(&rx_telem_queue, &lora_packet, K_MSEC(100)) == 0) {
            l_send_udp_broadcast(sock, lora_packet.payload, lora_packet.payload_len, lora_packet.port);
            gpio_pin_toggle_dt(&led0);
        }

        if (k_msgq_get(&statistics_queue, &lora_statistics, K_MSEC(100)) == 0) {
            l_send_udp_broadcast(sock, (uint8_t *) &lora_statistics, sizeof(l_lora_statistics_t),
                                 RADIO_MODULE_BASE_PORT);
            gpio_pin_toggle_dt(&led1);
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

    LOG_INF("Received payload of %d bytes for port %d", len - 2, payload[0] << 8 | payload[1]);

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
    return 0;
}

int main_unique() {
    LOG_INF("Started radio module RECEIVER");
    return 0;
}

#endif
