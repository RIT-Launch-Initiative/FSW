#ifndef CONFIG_RADIO_MODULE_RECEIVER

// Self Include
#include "radio_module_functionality.h"

// Launch Includes
#include <launch_core/backplane_defs.h>
#include <launch_core/dev/gnss.h>
#include <launch_core/types.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(radio_module_txer);

// Flags
static bool ready_to_tx = false;

// Forward Declaration
static void gnss_data_cb(const struct device *dev, const struct gnss_data *data);

// Callbacks
GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnss_data_cb);
// GNSS_SATELLITES_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), l_gnss_debug_sat_count_cb);

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

#ifdef CONFIG_DEBUG
#define UDP_TX_QUEUE_SIZE 8
K_MSGQ_DEFINE(udp_tx_queue, sizeof(l_gnss_data_t), UDP_TX_QUEUE_SIZE, 1);
#endif

// Timers
struct k_timer gnss_tx_timer;
static void gnss_tx_on_expire(struct k_timer *timer_id); // Forward Declaration
K_TIMER_DEFINE(gnss_tx_timer, gnss_tx_on_expire, NULL);

// Threads
static K_THREAD_STACK_DEFINE(udp_rx_stack, UDP_RX_STACK_SIZE);
static struct k_thread udp_rx_thread;

static K_THREAD_STACK_DEFINE(lora_tx_stack, LORA_TX_STACK_SIZE);
static struct k_thread lora_tx_thread;

#ifdef CONFIG_DEBUG
#define UDP_TX_STACK_SIZE 1024
static K_THREAD_STACK_DEFINE(udp_tx_stack, UDP_TX_STACK_SIZE);
static struct k_thread udp_tx_thread;
#endif

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

#ifdef CONFIG_DEBUG
static void udp_tx_task(void *socks, void *unused1, void *unused2) {
    l_udp_socket_list_t const *sock_list = (l_udp_socket_list_t *) socks;
    const uint16_t gnss_port = RADIO_MODULE_BASE_PORT + RADIO_MODULE_GNSS_DATA_PORT;

    while (1) {
        l_gnss_data_t gnss_data = {0};
        k_msgq_get(&udp_tx_queue, &gnss_data, K_FOREVER);
        /// TODO: change this socket number later once we figure out how to add more
        for (int s = 0; s < sock_list->num_sockets; s++) {
            l_send_udp_broadcast(sock_list->sockets[s], (uint8_t *) &gnss_data, sizeof(l_gnss_data_t), gnss_port);
        }
    }
}
#endif

static void lora_tx_task(void *, void *, void *) {
    const struct device *const lora_dev = DEVICE_DT_GET_ONE(semtech_sx1276);

    while (1) {
        l_lora_packet_t packet = {0};
        k_msgq_get(&lora_tx_queue, &packet, K_FOREVER);
        l_lora_tx(lora_dev, (uint8_t *) &packet, packet.payload_len + sizeof(packet.port));
    }
}

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data) {
    if (!ready_to_tx) {
        return; // timer hasnt expired yet
    }
    l_lora_packet_t packet = {0};
    packet.port = RADIO_MODULE_GNSS_DATA_PORT + RADIO_MODULE_BASE_PORT;
    packet.payload_len = sizeof(l_gnss_data_t);

    l_gnss_data_t gnss_data = {0};
    gnss_data.latitude = (double) data->nav_data.latitude / (double) L_GNSS_LATITUDE_DIVISION_FACTOR;
    gnss_data.longitude = (double) data->nav_data.longitude / (double) L_GNSS_LONGITUDE_DIVISION_FACTOR;
    gnss_data.altitude = (float) data->nav_data.altitude / L_GNSS_ALTITUDE_DIVISION_FACTOR;

    memcpy(packet.payload, &gnss_data, sizeof(l_gnss_data_t));
    k_msgq_put(&lora_tx_queue, (void *) &packet, K_NO_WAIT);

#ifdef CONFIG_DEBUG // if debugging is on tx gnss over ethernet
    // push to udp tx queue
    k_msgq_put(&udp_tx_queue, (void *) &gnss_data, K_NO_WAIT);
#endif

    ready_to_tx = false;
}

static void gnss_tx_on_expire(struct k_timer *timer_id) { ready_to_tx = true; }

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

#ifdef CONFIG_DEBUG
    k_thread_create(&udp_tx_thread, &udp_tx_stack[0], UDP_TX_STACK_SIZE, udp_tx_task, &udp_socket_list, NULL, NULL,
                    K_PRIO_PREEMPT(6), 0, K_NO_WAIT);
    k_thread_start(&udp_tx_thread);
#endif

    return 0;
}

int start_tasks() {
    k_thread_create(&lora_tx_thread, &lora_tx_stack[0], LORA_TX_STACK_SIZE, lora_tx_task, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
    k_thread_start(&lora_tx_thread);
    k_timer_start(&gnss_tx_timer, K_MSEC(CONFIG_GNSS_DATA_TX_INTERVAL), K_MSEC(CONFIG_GNSS_DATA_TX_INTERVAL));
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