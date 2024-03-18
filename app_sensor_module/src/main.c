#include <launch_core/backplane_defs.h>
#include <launch_core/types.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define STACK_SIZE (512)

LOG_MODULE_REGISTER(main, CONFIG_APP_SENSOR_MODULE_LOG_LEVEL);

// Queues
static struct k_msgq ten_hz_telemetry_queue;
static uint8_t ten_hz_telemetry_queue_buffer[CONFIG_TEN_HZ_QUEUE_SIZE * sizeof(sensor_module_ten_hz_telemetry_t)];

static struct k_msgq hundred_hz_telemetry_queue;
static uint8_t hundred_hz_telemetry_queue_buffer[
        CONFIG_HUNDRED_HZ_QUEUE_SIZE * sizeof(sensor_module_hundred_hz_telemetry_t)];

// Threads
static K_THREAD_STACK_DEFINE(telemetry_processing_stack, STACK_SIZE);
static struct k_thread telemetry_processing_thread;

// Devices
//#define LED0_NODE DT_ALIAS(led0)
//static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
//
//#define LED1_NODE DT_ALIAS(led1)
//static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

static void telemetry_processing_task(void *, void *, void *) {
    sensor_module_ten_hz_telemetry_t ten_hz_telem;
    sensor_module_hundred_hz_telemetry_t hundred_hz_telem;

    int ten_hz_socket = l_init_udp_socket(SENSOR_MODULE_IP_ADDR, SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT);
    int hundred_hz_socket = l_init_udp_socket(SENSOR_MODULE_IP_ADDR, SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);

    while (true) {
        if (0 == k_msgq_get(&ten_hz_telemetry_queue, &ten_hz_telem, K_NO_WAIT)) {
            l_send_udp_broadcast(ten_hz_socket, (uint8_t *) &ten_hz_telem, sizeof(sensor_module_ten_hz_telemetry_t),
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT);
        } else {
            LOG_WRN("Failed to get data from 10 Hz queue");
        }

        if (0 == k_msgq_get(&hundred_hz_telemetry_queue, &hundred_hz_telem, K_NO_WAIT)) {
            l_send_udp_broadcast(hundred_hz_socket, (uint8_t *) &hundred_hz_telem, sizeof(sensor_module_hundred_hz_telemetry_t),
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);
        } else {
            LOG_WRN("Failed to get data from 100 Hz queue");
        }

        // TODO: Extension board support. Need to figure out a robust way of doing this

        // TODO: write to flash when data logging library is ready
    }
}

static int init(void) {
    char ip[MAX_IP_ADDRESS_STR_LEN];
    int ret = 0;

    k_msgq_init(&ten_hz_telemetry_queue, ten_hz_telemetry_queue_buffer, sizeof(sensor_module_ten_hz_telemetry_t),
                CONFIG_TEN_HZ_QUEUE_SIZE);
    k_msgq_init(&hundred_hz_telemetry_queue, hundred_hz_telemetry_queue_buffer,
                sizeof(sensor_module_hundred_hz_telemetry_t),
                CONFIG_HUNDRED_HZ_QUEUE_SIZE);

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

    // Tasks
    k_thread_create(&telemetry_processing_thread, &telemetry_processing_stack[0], STACK_SIZE,
                    telemetry_processing_task, NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
    k_thread_start(&telemetry_processing_thread);

    return 0;
}


int main() {
    if (!init()) {
        return -1;
    }

    while (1) {
    }

    return 0;
}


