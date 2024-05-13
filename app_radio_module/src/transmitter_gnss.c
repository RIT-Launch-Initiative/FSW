#include "transmitter_gnss.h"

#include "radio_module_functionality.h"

// Launch Includes
#include <launch_core/dev/gnss.h>
#include <launch_core/types.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(transmitter_gnss);

// Flags
static bool ready_to_tx = false;
bool logging_enabled = false;

// Forward Declaration
static void gnss_data_cb(const struct device *dev, const struct gnss_data *data);
static void gnss_log_task(void);

// Callbacks
GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnss_data_cb);
// GNSS_SATELLITES_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), l_gnss_debug_sat_count_cb);

// Timers
struct k_timer gnss_tx_timer;

// External Variables
extern struct k_msgq lora_tx_queue;
float gnss_altitude;

static void gnss_tx_on_expire(struct k_timer *timer_id); // Forward Declaration
K_TIMER_DEFINE(gnss_tx_timer, gnss_tx_on_expire, NULL);

#ifdef CONFIG_DEBUG

#define GNSS_TX_QUEUE_SIZE 8
#define GNSS_TASK_STACK_SIZE 1024

static void gnss_debug_task(void);

K_MSGQ_DEFINE(gnss_tx_queue, sizeof(l_gnss_data_t), GNSS_TX_QUEUE_SIZE, 1);
K_THREAD_DEFINE(gnss_udp_tx, GNSS_TASK_STACK_SIZE, gnss_debug_task, NULL, NULL, NULL, K_PRIO_PREEMPT(25), 0, 1000);

K_MSGQ_DEFINE(gnss_log_queue, sizeof(l_gnss_data_t), CONFIG_LORA_TX_QUEUE_SIZE, 1);
K_THREAD_DEFINE(gnss_log_thread, GNSS_TASK_STACK_SIZE, gnss_log_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);


static void gnss_debug_task(void) {
    const uint16_t gnss_port = RADIO_MODULE_BASE_PORT + RADIO_MODULE_GNSS_DATA_PORT;
    int sock = l_init_udp_socket(RADIO_MODULE_IP_ADDR, gnss_port);
    if (sock < 0) {
        LOG_ERR("Failed to initialize UDP socket for GNSS debug task");
        return;
    }

    while (1) {
        l_gnss_data_t gnss_data = {0};
        k_msgq_get(&gnss_tx_queue, &gnss_data, K_FOREVER);
        l_send_udp_broadcast(sock, (uint8_t *) &gnss_data, sizeof(l_gnss_data_t), gnss_port);
    }
}

#endif

void config_gnss_tx_time(k_timeout_t interval) { k_timer_start(&gnss_tx_timer, interval, interval); }

static void gnss_tx_on_expire(struct k_timer *timer_id) { ready_to_tx = true; }

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data) {
    gnss_altitude = (float) data->nav_data.altitude / L_GNSS_ALTITUDE_DIVISION_FACTOR;

    if (!ready_to_tx) {
        return; // timer hasnt expired yet
    }
    l_lora_packet_t packet = {0};
    packet.port = RADIO_MODULE_GNSS_DATA_PORT + RADIO_MODULE_BASE_PORT;
    packet.payload_len = sizeof(l_gnss_data_t);

    l_gnss_data_t gnss_data = {0};
    gnss_data.latitude = (double) data->nav_data.latitude / (double) L_GNSS_LATITUDE_DIVISION_FACTOR;
    gnss_data.longitude = (double) data->nav_data.longitude / (double) L_GNSS_LONGITUDE_DIVISION_FACTOR;
    gnss_data.altitude = gnss_altitude;

    // TODO: Eventually make this zbus and use a single bus for both lora and gnss queues to share
    memcpy(packet.payload, &gnss_data, sizeof(l_gnss_data_t));
    k_msgq_put(&lora_tx_queue, &packet, K_NO_WAIT);
    if (logging_enabled) {
        k_msgq_put(&gnss_log_queue, &packet, K_NO_WAIT);
    }

#ifdef CONFIG_DEBUG // if debugging is on tx gnss over ethernet
    // push to udp tx queue
    k_msgq_put(&gnss_tx_queue, (void *) &gnss_data, K_NO_WAIT);
#endif

    ready_to_tx = false;
}

static void gnss_log_task(void) {
    l_gnss_data_t data = {};
    while (1) {
        if (k_msgq_get(&gnss_log_queue, &data, K_FOREVER)) {
            // TODO: Implement once logging is done
        }

    }
}