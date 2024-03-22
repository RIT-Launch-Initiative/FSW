#if defined(RADIO_MODULE_RECEIVER)
#include "radio_module_functionality.h"

l_udp_socket_list_t udp_socket_list = {
        .sockets = NULL,
        .ports = NULL,
        .num_sockets = 0
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
