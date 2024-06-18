// Self Include
#include "sensor_module.h"

// Launch Includes
#include <launch_core/dev/dev_common.h>
#include <launch_core/types.h>

// Zephyr Includes
#include <launch_core/backplane_defs.h>
#include <launch_core/net/udp.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(networking);

static void telemetry_broadcast_task(void*, void*, void*);

K_THREAD_DEFINE(telemetry_broadcast, 1024, telemetry_broadcast_task, NULL, NULL, NULL, K_PRIO_PREEMPT(20), 0, 1000);

extern struct k_msgq hundred_hz_telem_queue;

static const struct device* uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart5));

int init_networking(void) {
    if (l_check_device(DEVICE_DT_GET_ONE(wiznet_w5500)) != 0) {
        LOG_ERR("Wiznet device not found");
        return -ENODEV;
    }

    int ret = l_init_udp_net_stack_default(SENSOR_MODULE_IP_ADDR);
    if (ret != 0) {
        LOG_ERR("Failed to initialize UDP networking stack: %d", ret);
        return -ENXIO;
    }

    return 0;
}

//static void initialize_rs485(void) {
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
//}

static int32_t read_potato_data() {
    static const struct gpio_dt_spec rs485_enable = GPIO_DT_SPEC_GET(DT_ALIAS(de), gpios);
    gpio_pin_set_dt(&rs485_enable, 0);
    int32_t result = 0;
    LOG_INF("Starting POTATO Read");
    uint8_t byte = 0;

    while (byte != '\n') {
        while(uart_poll_in(uart_dev, &byte));
        LOG_INF("Got %x", byte);
    }

    while (uart_poll_in(uart_dev, &byte));
    result |= byte;

    while (uart_poll_in(uart_dev, &byte));
    result |= byte << 8;

    while (uart_poll_in(uart_dev, &byte));
    result |= byte << 16;

    LOG_INF("Got %d", result);

    return result;
}

static void telemetry_broadcast_task(void*, void*, void*) {
    LOG_INF("Starting broadcast task");

    timed_sensor_module_hundred_hz_telemetry_t hundred_hz_telem;

    int hundred_hz_socket =
        l_init_udp_socket(SENSOR_MODULE_IP_ADDR, SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);

#ifdef CONFIG_IREC_2024_DEMO

    int potato_socket = l_init_udp_socket(SENSOR_MODULE_IP_ADDR, SENSOR_MODULE_BASE_PORT);
#endif

    while (true) {
        // if (0 == k_msgq_get(&hundred_hz_telem_queue, &hundred_hz_telem, K_MSEC(100))) {
        //     l_send_udp_broadcast(hundred_hz_socket, (uint8_t*) &hundred_hz_telem,
        //                          sizeof(sensor_module_hundred_hz_telemetry_t),
        //                          SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);

#ifdef CONFIG_IREC_2024_DEMO
        // TODO(aaron) read potato data
        // l_send_udp_broadcast(potato_socket, (const uint8_t*) potato_data, sizeof(potato_data), SENSOR_MODULE_BASE_PORT);

        read_potato_data();
#endif
    }
}

