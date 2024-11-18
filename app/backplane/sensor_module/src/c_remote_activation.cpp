#include "c_remote_activation.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CRemoteActivationTenant);

void CRemoteActivationTenant::Startup() {};

void CRemoteActivationTenant::PostStartup() {};

void CRemoteActivationTenant::Run() {
    gpio_pin_toggle_dt(&pin);
}