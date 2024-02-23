#include "sensors.h"
#include <launch_core/backplane_defs.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_SENSOR_MODULE_LOG_LEVEL);

static struct k_msgq ten_hz_telemetry_queue;
static uint8_t ten_hz_telemetry_queue_buffer[CONFIG_TEN_HZ_QUEUE_SIZE * sizeof(float)];

static struct k_msgq hundred_hz_telemetry_queue;
static uint8_t hundred_hz_telemetry_queue_buffer[CONFIG_HUNDRED_HZ_QUEUE_SIZE * sizeof(sensor_module_telemetry_t)];

static struct k_msgq extension_board_telemetry_queue;
static uint8_t extension_board_telemetry_queue_buffer[CONFIG_EXTENSION_BOARD_QUEUE_SIZE * 16]; // TODO: Come up with a good size

#define STACK_SIZE (512)
//#define LED0_NODE DT_ALIAS(led0)
//static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
//
//#define LED1_NODE DT_ALIAS(led1)
//static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
//static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
//static const struct device *const ms5611 = DEVICE_DT_GET_ONE(meas_ms5611);

//static const struct device *const adxl375 = DEVICE_DT_GET_ONE(adi_adxl375);

static int init(void) {
    char ip[MAX_IP_ADDRESS_STR_LEN];

    k_queue_init(&ten_hz_telemetry_queue);
    k_queue_init(&hundred_hz_telemetry_queue);
    k_queue_init(&extension_board_telemetry_queue);
    int ret = 0;

    if (0 > l_create_ip_str_default_net_id(ip, SENSOR_MODULE_ID, 1)) {
        LOG_ERR("Failed to create IP address string: %d", ret);
        return -1;
    }


    if (!l_check_device(DEVICE_DT_GET_ONE(wiznet_w5500))) {
        if (!l_init_udp_net_stack(ip)) {
            LOG_ERR("Failed to initialize network stack");
        }
    } else {
        LOG_ERR("Failed to get network device");
    }

    // Sensors

    return 0;
}

static void telemetry_queue_processing_task(void *, void *, void *) {
    sensor_module_telemetry_t sensor_telemetry = {0};
    sensor_module_telemetry_packed_t packed_telemetry = {0};

    while (true) {
        if (0 == k_msgq_get(&hundred_hz_telemetry_queue, &sensor_telemetry, K_USEC(10))) {
            l_send_udp_broadcast((uint8_t * ) &packed_telemetry, sizeof(sensor_module_telemetry_packed_t),
                             SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);
        } else {
            LOG_WRN("Failed to get data from 100 Hz queue");
        }

        if (0 == k_msgq_get(&ten_hz_telemetry_queue, &sensor_telemetry, K_USEC(10))) { // Don't want to hold up processing above 
            l_send_udp_broadcast((uint8_t *) &packed_telemetry, sizeof(sensor_module_telemetry_packed_t),
                             SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT);
        } else {
            LOG_WRN("Failed to get data from 10 Hz queue");
        }
        
        if (0 == k_msgq_get(&extension_board_telemetry_queue, &sensor_telemetry, K_USEC(10))) { // Don't want to hold up processing above 
            l_send_udp_broadcast((uint8_t *) &packed_telemetry, sizeof(64),
                             SENSOR_MODULE_BASE_PORT);
        } else {
            LOG_WRN("Failed to get data from 10 Hz queue");
        }
        // TODO: write to flash when data logging library is ready
    }
}


int main() {
    if (!init()) {
        return -1;
    }

    while (1) {
    }

    return 0;
}


