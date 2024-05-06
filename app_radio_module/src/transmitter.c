#ifndef CONFIG_RADIO_MODULE_RECEIVER

// Self Include
#include "radio_module_functionality.h"

#include "transmitter_gnss.h"
#include "transmitter_smf.h"

// Launch Includes
#include <launch_core/backplane_defs.h>
#include <launch_core/types.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(radio_module_txer);

static uint8_t udp_rx_buffer[UDP_RX_BUFF_LEN];

static int udp_sockets[NUM_SOCKETS] = {0};
static uint16_t udp_socket_ports[NUM_SOCKETS] = {
    LAUNCH_EVENT_NOTIFICATION_PORT,
    POWER_MODULE_BASE_PORT + POWER_MODULE_INA_DATA_PORT,
    SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT,
    SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT,
};

l_udp_socket_list_t udp_socket_list = {.sockets = udp_sockets, .ports = udp_socket_ports, .num_sockets = NUM_SOCKETS};

// Queues
K_MSGQ_DEFINE(lora_tx_queue, sizeof(l_lora_packet_t), CONFIG_LORA_TX_QUEUE_SIZE, 1);

// Threads
static K_THREAD_STACK_DEFINE(udp_rx_stack, UDP_RX_STACK_SIZE);
static struct k_thread udp_rx_thread;

static K_THREAD_STACK_DEFINE(lora_tx_stack, LORA_TX_STACK_SIZE);
static struct k_thread lora_tx_thread;

static void udp_rx_task(void *socks, void *buff_ptr, void *buff_len) {
    l_udp_socket_list_t const *sock_list = (l_udp_socket_list_t *) socks;
    size_t buff_size = POINTER_TO_INT(buff_len);
    int rcv_size = 0;

    while (true) {
        for (int i = 0; i < sock_list->num_sockets; i++) {
            l_lora_packet_t packet = {0};

            packet.port = sock_list->ports[i];
            rcv_size = l_receive_udp(sock_list->sockets[i], packet.payload, buff_size);
            if (rcv_size <= 0) {
                continue;
            }

            packet.payload_len = (uint8_t) rcv_size;
            k_msgq_put(&lora_tx_queue, &packet, K_NO_WAIT);
            LOG_INF("Finished putting on queue");
        }
    }
}

static void lora_tx_task(void *, void *, void *) {
    const struct device *const lora_dev = DEVICE_DT_GET_ONE(semtech_sx1276);

    while (1) {
        l_lora_packet_t packet = {0};
        k_msgq_get(&lora_tx_queue, &packet, K_FOREVER);
        l_lora_tx(lora_dev, (uint8_t *) &packet, packet.payload_len + sizeof(packet.port));
    }
}

int init_lora_unique(const struct device *const lora_dev) { return l_lora_set_tx_rx(lora_dev, true); }

int init_udp_unique() {
    for (int i = 0; i < udp_socket_list.num_sockets; i++) {
        udp_socket_list.sockets[i] = l_init_udp_socket(RADIO_MODULE_IP_ADDR, udp_socket_list.ports[i]);
        if (udp_socket_list.sockets[i] < 0) {
            LOG_ERR("Failed to create UDP socket: %d", udp_socket_list.sockets[i]);
        } else {
            l_set_socket_rx_timeout(udp_socket_list.sockets[i], 1);
        }
    }

    k_thread_create(&udp_rx_thread, &udp_rx_stack[0], UDP_RX_STACK_SIZE, udp_rx_task, &udp_socket_list, udp_rx_buffer,
                    INT_TO_POINTER(UDP_RX_BUFF_LEN), K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
    k_thread_start(&udp_rx_thread);

    return 0;
}

int start_tasks() {
    k_thread_create(&lora_tx_thread, &lora_tx_stack[0], LORA_TX_STACK_SIZE, lora_tx_task, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
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