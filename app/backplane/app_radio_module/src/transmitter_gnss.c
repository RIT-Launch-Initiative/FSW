#include <zephyr/drivers/gpio.h>
#ifndef CONFIG_RADIO_MODULE_RECEIVER
#include "transmitter_gnss.h"

#include "radio_module_functionality.h"

// Launch Includes
#include <launch_core_classic/dev/gnss.h>
#include <launch_core_classic/types.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define GNSS_TX_QUEUE_SIZE 8
#define GNSS_TASK_STACK_SIZE 1024

LOG_MODULE_REGISTER(transmitter_gnss);

// Forward Declaration
static void gnss_tx_on_expire(struct k_timer* timer_id);
static void gnss_data_cb(const struct device* dev, const struct gnss_data* data);

// Message Queues
K_MSGQ_DEFINE(gnss_tx_queue, sizeof(l_gnss_data_t), GNSS_TX_QUEUE_SIZE, 1);

// Flags
static bool ready_to_tx = false;
bool logging_enabled = false;

// LEDs (can't be defined in cb)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

// Callbacks
GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnss_data_cb);
// GNSS_SATELLITES_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), l_gnss_debug_sat_count_cb);

// Timers
struct k_timer gnss_tx_timer;
K_TIMER_DEFINE(gnss_tx_timer, gnss_tx_on_expire, NULL);

// External Variables
extern struct k_msgq lora_tx_queue;
extern struct k_msgq gnss_logging_msgq;
float gnss_altitude;

void config_gnss_tx_time(k_timeout_t interval) { k_timer_start(&gnss_tx_timer, interval, interval); }

static void gnss_tx_on_expire(struct k_timer* timer_id) { ready_to_tx = true; }

static void gnss_data_cb(const struct device* dev, const struct gnss_data* data) {
    gpio_pin_toggle_dt(&led0);
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

    LOG_INF("Queuing GNSS packet");
    memcpy(packet.payload, &gnss_data, sizeof(l_gnss_data_t));
    k_msgq_put(&lora_tx_queue, &packet, K_NO_WAIT);
    if (logging_enabled) {
        k_msgq_put(&gnss_logging_msgq, &packet, K_NO_WAIT);
    }

#ifdef CONFIG_DEBUG // if debugging is on tx gnss over ethernet
    // push to udp tx queue
    k_msgq_put(&gnss_tx_queue, (void*) &gnss_data, K_NO_WAIT);
#endif

    ready_to_tx = false;
}

#ifdef CONFIG_DEBUG
static void gnss_debug_task(void);
K_THREAD_DEFINE(gnss_udp_tx, GNSS_TASK_STACK_SIZE, gnss_debug_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

static void gnss_debug_task(void) {
    LOG_INF("Started GNSS Debug task");
    const uint16_t gnss_port = RADIO_MODULE_BASE_PORT + RADIO_MODULE_GNSS_DATA_PORT;
    int sock = l_init_udp_socket(RADIO_MODULE_IP_ADDR, gnss_port);
    if (sock < 0) {
        LOG_ERR("Failed to initialize UDP socket for GNSS debug task");
        return;
    }

    while (true) {
        l_gnss_data_t gnss_data = {0};
        k_msgq_get(&gnss_tx_queue, &gnss_data, K_FOREVER);
        l_send_udp_broadcast(sock, (uint8_t*) &gnss_data, sizeof(l_gnss_data_t), gnss_port);
    }
}

#endif
#endif
