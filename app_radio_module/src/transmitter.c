#if !defined(RADIO_MODULE_RECEIVER)

#include <launch_core/dev/gnss.h>
#include "radio_module_functionality.h"

#define NUM_SOCKETS 4

// Callbacks
GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), l_gnss_data_debug_cb);
GNSS_SATELLITES_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), l_gnss_debug_sat_count_cb);

// UDP Configuration
static int udp_sockets[NUM_SOCKETS] = {0};
static int udp_socket_ports[NUM_SOCKETS] = {LAUNCH_EVENT_NOTIFICATION_PORT,
                                 POWER_MODULE_BASE_PORT + POWER_MODULE_INA_DATA_PORT,
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_TEN_HZ_DATA_PORT,
                                 SENSOR_MODULE_BASE_PORT + SENSOR_MODULE_HUNDRED_HZ_DATA_PORT,
                                 };
l_udp_socket_list_t udp_socket_list = {
        .sockets = udp_sockets,
        .ports = udp_socket_ports,
        .num_sockets = NUM_SOCKETS
};


int init_lora_unique() {
    return 0;
}

int init_udp_unique() {
    return 0;
}

int start_tasks() {


    return 0;
}

#endif
