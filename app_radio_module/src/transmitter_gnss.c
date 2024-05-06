#include "transmitter_gnss.h"
#include "radio_module_functionality.h"

// Launch Includes
#include <launch_core/dev/gnss.h>
#include <launch_core/types.h>


// Zephyr Includes
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

// Timers
struct k_timer gnss_tx_timer;

static void gnss_tx_on_expire(struct k_timer *timer_id); // Forward Declaration
K_TIMER_DEFINE(gnss_tx_timer, gnss_tx_on_expire, NULL);


void init_gnss(void) {
    k_timer_start(&gnss_tx_timer, K_MSEC(CONFIG_GNSS_DATA_TX_INTERVAL), K_MSEC(CONFIG_GNSS_DATA_TX_INTERVAL));
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

#ifdef CONFIG_DEBUG
#define UDP_TX_QUEUE_SIZE 8
K_MSGQ_DEFINE(udp_tx_queue, sizeof(l_gnss_data_t), UDP_TX_QUEUE_SIZE, 1);
#define UDP_TX_STACK_SIZE 1024
static K_THREAD_STACK_DEFINE(udp_tx_stack, UDP_TX_STACK_SIZE);
static struct k_thread udp_tx_thread;
#endif

static void gnss_debug_task(void *socks, void *unused1, void *unused2) {
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