#include "networking.h"


#include <launch_core/types.h>
#include <launch_core/dev/dev_common.h>

#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(networking);


// Sockets
static int udp_sockets[1] = {0};
static int udp_socket_ports[1] = {SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT};
static l_udp_socket_list_t udp_socket_list = {
        .sockets = udp_sockets,
        .num_sockets = 1
};

static void telemetry_broadcast_task(void *, void *, void *);

K_THREAD_DEFINE(telemetry_broadcast, 1024, telemetry_broadcast_task, NULL, NULL, NULL, K_PRIO_PREEMPT(15), 0, 0);

extern struct k_msgq hundred_hz_telem_queue;

static int initialize_eth(void) {
    if (l_check_device(DEVICE_DT_GET_ONE(wiznet_w5500)) != 0) {
        LOG_ERR("Wiznet device not found");
        return -ENODEV;
    }

    int ret = l_init_udp_net_stack_default(SENSOR_MODULE_IP_ADDR);
    if (ret != 0) {
        LOG_ERR("Failed to initialize UDP networking stack: %d", ret);
        return -ENXIO;
    }

    for (int i = 0; i < udp_socket_list.num_sockets; i++) {
        udp_socket_list.sockets[i] = l_init_udp_socket(SENSOR_MODULE_IP_ADDR, udp_socket_ports[i]);
        if (udp_socket_list.sockets[i] < 0) {
            LOG_ERR("Failed to create UDP socket: %d", udp_socket_list.sockets[i]);
        }
    }

    return 0;
}

static void initialize_rs485(void) {
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



static void telemetry_broadcast_task(void *, void *, void *) {
    if (initialize_eth()) {
        LOG_ERR("Failed to initialize Ethernet");
        return;
    }

    while (true) {
        sensor_module_ten_hz_telemetry_t ten_hz_telem;
        sensor_module_hundred_hz_telemetry_t hundred_hz_telem;

        int ten_hz_socket = l_init_udp_socket(SENSOR_MODULE_IP_ADDR,
                                              SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT);
        int hundred_hz_socket = l_init_udp_socket(SENSOR_MODULE_IP_ADDR,
                                                  SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT);

        while (true) {
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
}
