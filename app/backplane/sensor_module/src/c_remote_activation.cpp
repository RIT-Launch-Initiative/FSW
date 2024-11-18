#include "c_remote_activation.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CRemoteActivationTenant);

void CRemoteActivationTenant::Startup() {};

void CRemoteActivationTenant::PostStartup() {};

void CRemoteActivationTenant::Run() {
    uint8_t command = 0;
    if (udp.ReceiveAsynchronous(&command, sizeof(command)) < 0) {
        LOG_ERR("Failed to receive command");
        return;
    }

    // Check bit 0 set
    if (command & 0b1) {
        gpio_pin_toggle_dt(&pin);
    } else if (!(command & 0b1)) {
        gpio_pin_set_dt(&pin, 0);
    }
}