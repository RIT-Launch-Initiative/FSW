#ifdef CONFIG_RADIO_MODULE_RECEIVER

// Self Include
#include "radio_module_functionality.h"

// Launch Includes
#include <launch_core/net/lora.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>

int init_lora_unique(const struct device *const lora_dev) {
    return l_lora_set_tx_rx(lora_dev, false);
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
