#ifdef CONFIG_RADIO_MODULE_RECEIVER

// Self Include
#include "radio_module_functionality.h"

// Launch Includes
#include <launch_core_classic/net/lora.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(radio_module_rxer);

// Queues - Name, Entry Size, Queue Size, Alignment
K_MSGQ_DEFINE(rx_telem_queue, sizeof(l_lora_packet_t), 8, 1);
K_MSGQ_DEFINE(statistics_queue, sizeof(l_lora_statistics_t), 8, 1);

static void reset_recv_async(struct k_timer*);
K_TIMER_DEFINE(recv_heartbeat, reset_recv_async, NULL);

// LEDs
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct device* lora;

// Forward Declares
static void udp_broadcast_task(void);

// Threads
#define THREAD_STACK_SIZE 2048
K_THREAD_DEFINE(udp_bcast_thread, THREAD_STACK_SIZE, udp_broadcast_task, NULL, NULL, NULL, K_PRIO_PREEMPT(25), 0, 2000);
int sock = -1;

static void init_udp_sock() {
    if (sock == -1) {
        sock = l_init_udp_socket(RADIO_MODULE_IP_ADDR, RADIO_MODULE_BASE_PORT);
        if (sock >= 0) {
            l_set_socket_rx_timeout(sock, 10);
        }
    }
}

static void udp_broadcast_task(void) {
    init_udp_sock();

    if (sock < 0) {
        LOG_ERR("Failed to create socket for UDP broadcasts. Not sending received LoRa packets");
        return;
    }

    l_lora_packet_t lora_packet = {0};
    l_lora_statistics_t lora_statistics = {0};

    while (true) {
        if (k_msgq_get(&rx_telem_queue, &lora_packet, K_MSEC(100)) == 0) {
            LOG_INF("Received for port %d", lora_packet.port);
            l_send_udp_broadcast(sock, lora_packet.payload, lora_packet.payload_len, lora_packet.port);
            gpio_pin_toggle_dt(&led0);
        }

        if (k_msgq_get(&statistics_queue, &lora_statistics, K_MSEC(100)) == 0) {
            l_send_udp_broadcast(sock, (uint8_t*) &lora_statistics, sizeof(l_lora_statistics_t),
                                 RADIO_MODULE_BASE_PORT);
        }
    }
}

static void receiver_cb(const struct device* lora_dev, uint8_t* payload, uint16_t len, int16_t rssi, int8_t snr) {
    ARG_UNUSED(lora_dev);
    ARG_UNUSED(len);

    gpio_pin_toggle_dt(&led0);
    // Reset the heartbeat timer
    k_timer_start(&recv_heartbeat, K_MSEC(10000), K_NO_WAIT);

    static l_lora_statistics_t statistics = {0};
    statistics.count++;
    statistics.rssi = rssi;
    statistics.snr = snr;

    l_lora_packet_t lora_packet = {
        .port = sys_get_le16(payload), // Get the first 2 bytes as the port
        .payload_len = len - 2         // Subtract the 2 bytes used for the port
    };

    memcpy(lora_packet.payload, payload + 2, lora_packet.payload_len);

    LOG_INF("Received payload of %d bytes for port %d", lora_packet.payload_len, lora_packet.port);

    if (k_msgq_put(&rx_telem_queue, (void*) &lora_packet, K_NO_WAIT) < 0) {
        LOG_ERR("Failed to queue received telemetry");
    }

    if (k_msgq_put(&statistics_queue, &statistics, K_NO_WAIT) < 0) {
        LOG_ERR("Failed to queue statistics");
    }

    LOG_INF("Received %d bytes. RSSI: %d SNR: %d", len, rssi, snr);
}

static const struct device* lora = NULL;
static void reset_recv_async(struct k_timer*) {
    LOG_INF("Resetting recv async!");
    // lora_recv_async(lora, NULL);
    // k_timer_start(&recv_heartbeat, K_SECONDS(10), K_SECONDS(10));
    // lora_recv_async(lora, &receiver_cb);
    gpio_pin_toggle_dt(&led1);
}


int init_lora_unique(const struct device* const lora_dev) {
    lora = lora_dev;
    return lora_recv_async(lora, &receiver_cb);
    // reset_recv_async(NULL);
    // return 0;
}

int init_udp_unique() {
    return 0;
}

int main_unique() {
    LOG_INF("Started radio module RECEIVER");
    return 0;
}

#endif
