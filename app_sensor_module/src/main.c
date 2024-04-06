#include "sensors.h"

#include <launch_core/backplane_defs.h>
#include <launch_core/types.h>
#include <launch_core/dev/dev_common.h>
#include <launch_core/dev/uart.h>
#include <launch_core/net/net_common.h>
#include <launch_core/net/udp.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define SENSOR_MODULE_IP_ADDR BACKPLANE_IP(SENSOR_MODULE_ID, 2, 1) // TODO: KConfig the board revision and #
#define STACK_SIZE (512)

LOG_MODULE_REGISTER(main, CONFIG_APP_SENSOR_MODULE_LOG_LEVEL);

// Queues
static struct k_msgq ten_hz_telemetry_queue;
static uint8_t ten_hz_telemetry_queue_buffer[CONFIG_TEN_HZ_QUEUE_SIZE * sizeof(sensor_module_ten_hz_telemetry_t)];

extern struct k_msgq hundred_hz_telem_queue;
// Threads
static K_THREAD_STACK_DEFINE(telemetry_processing_stack, STACK_SIZE);
static struct k_thread telemetry_processing_thread;

// Devices
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

// Sockets
static int udp_sockets[1] = {0};
static int udp_socket_ports[1] = {SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT};
static l_udp_socket_list_t udp_socket_list = {
        .sockets = udp_sockets,
        .num_sockets = 1
};


static void telemetry_processing_task(void *, void *, void *) {
    sensor_module_ten_hz_telemetry_t ten_hz_telem;
    sensor_module_hundred_hz_telemetry_t hundred_hz_telem;

    int ten_hz_socket = l_init_udp_socket(SENSOR_MODULE_IP_ADDR,
                                          SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT);
    int hundred_hz_socket = l_init_udp_socket(SENSOR_MODULE_IP_ADDR,
                                              SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);

    while (true) {
        if (0 == k_msgq_get(&ten_hz_telemetry_queue, &ten_hz_telem, K_NO_WAIT)) {
            l_send_udp_broadcast(ten_hz_socket, (uint8_t *) &ten_hz_telem, sizeof(sensor_module_ten_hz_telemetry_t),
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT);
        } else {
            LOG_WRN("Failed to get data from 10 Hz queue");
        }

        if (0 == k_msgq_get(&hundred_hz_telem_queue, &hundred_hz_telem, K_NO_WAIT)) {
            l_send_udp_broadcast(hundred_hz_socket, (uint8_t *) &hundred_hz_telem,
                                 sizeof(sensor_module_hundred_hz_telemetry_t),
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);
        } else {
            LOG_WRN("Failed to get data from 100 Hz queue");
        }

        // TODO: Extension board support. Need to figure out a robust way of doing this

        // TODO: write to flash when data logging library is ready
    }
}

static void initialize_eth(void) {
    if (l_check_device(DEVICE_DT_GET_ONE(wiznet_w5500)) != 0) {
        LOG_ERR("Wiznet device not found");
        return;
    }

    int ret = l_init_udp_net_stack_default(SENSOR_MODULE_IP_ADDR);
    if (ret != 0) {
        LOG_ERR("Failed to initialize UDP networking stack: %d", ret);
        return;
    }

    for (int i = 0; i < udp_socket_list.num_sockets; i++) {
        udp_socket_list.sockets[i] = l_init_udp_socket(SENSOR_MODULE_IP_ADDR, udp_socket_ports[i]);
        if (udp_socket_list.sockets[i] < 0) {
            LOG_ERR("Failed to create UDP socket: %d", udp_socket_list.sockets[i]);
            return;
        }
    }
}

static void initialize_networks(void) {
    initialize_eth();

//    if (l_uart_init_rs485(DEVICE_DT_GET(DT_NODELABEL(uart5))) != 0) {
//        if (l_create_ip_str(rs485_ip, 11, 0, 3, 1) == 0) {
//            if (l_init_udp_net_stack_by_device(DEVICE_DT_GET(DT_NODELABEL(uart5)), rs485_ip)) {
//                LOG_ERR("Failed to initialize network stack");
//            }
//        } else {
//            LOG_ERR("Failed to create IP address string");
//        }
//    } else {
//        LOG_ERR("Failed to initialize UART to RS485");;
//    }
}

static int init(void) {
//    k_msgq_init(&ten_hz_telemetry_queue, ten_hz_telemetry_queue_buffer, sizeof(sensor_module_ten_hz_telemetry_t),
//                CONFIG_TEN_HZ_QUEUE_SIZE);


    initialize_networks();

    // Tasks
//    k_thread_create(&telemetry_processing_thread, &telemetry_processing_stack[0], STACK_SIZE,
//                    telemetry_processing_task, NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
//    k_thread_start(&telemetry_processing_thread);

    return 0;
}


int main() {
    if (init()) {
        return -1;
    }


    while (true) {
        l_send_udp_broadcast(udp_socket_list.sockets[0], "Launch!", 7,
                             SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT);
        gpio_pin_toggle_dt(&led0);
        gpio_pin_toggle_dt(&led1);
        k_msleep(100);
    }

    return 0;
}


