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

// LEDs
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct device* lora;

// Forward Declares
static void udp_broadcast_task(void);

static void udp_cmd_listener_task(void);

// Threads
#define THREAD_STACK_SIZE 2048
K_THREAD_DEFINE(udp_bcast_thread, THREAD_STACK_SIZE, udp_broadcast_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 2000);
K_THREAD_DEFINE(udp_cmd_thread, THREAD_STACK_SIZE, udp_cmd_listener_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 2000);
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
    static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
    static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

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
            gpio_pin_toggle_dt(&led1);
        }
    }
}

static void udp_cmd_listener_task(void) {
    init_udp_sock();

    while (true) {
        uint8_t buff[7] = {0};
        int rcv_size = l_receive_udp(sock, buff, sizeof(buff));
        if (rcv_size > 0) {
            LOG_INF("Received: %s", buff);
            if (strncmp(buff, "Launch!", 7)) {
                l_lora_tx(lora, buff, sizeof(buff));

                for (int i = 0; i < 10; i++) {
                    gpio_pin_toggle_dt(&led0);
                    gpio_pin_toggle_dt(&led1);
                    k_msleep(10);
                }

                break; // Should not transmit if we go into a launch state
            } else {
                LOG_WRN("Invalid command send to radio module. Ignoring...");
            }
        }

        k_msleep(100);
    }
}

static void receiver_cb(const struct device* lora_dev, uint8_t* payload, uint16_t len, int16_t rssi, int8_t snr) {
    ARG_UNUSED(lora_dev);
    ARG_UNUSED(len);

    gpio_pin_toggle_dt(&led0);

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
int init_lora_unique(const struct device* const lora_dev) {
    lora = lora_dev;
    // return lora_recv_async(lora_dev, &receiver_cb);
    return 0;
}

int init_udp_unique() {
    return 0;
}

int main_unique() {
    LOG_INF("Started radio module RECEIVER");

    while (true) {
        uint8_t data[255] = {0};
        int16_t rssi = 0;
        int8_t snr = 0;
        lora_recv(lora, data, sizeof(data), K_FOREVER, &rssi, &snr);
        receiver_cb(lora, data, sizeof(data), rssi, snr);
    }


    return 0;
}

#endif
