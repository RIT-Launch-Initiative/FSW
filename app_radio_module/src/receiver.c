#if defined(RADIO_MODULE_RECEIVER)
#include "radio_module_functionality.h"

int init_lora_unique(const struct device *const lora_dev) {
    return l_lora_configure(lora_dev, false);
}

int init_udp_unique(l_udp_socket_list_t *udp_socket_list) {
    return 0;
}

int start_tasks() {
    return 0;
}

int main_unique() {
    return 0;
}

#endif
