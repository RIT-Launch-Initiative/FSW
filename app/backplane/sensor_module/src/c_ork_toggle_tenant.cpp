#include "c_ork_toggle_tenant.h"

#include <errno.h>

#include <f_core/device/openrocket_launch.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(COrkToggleTenant);

COrkToggleTenant::COrkToggleTenant(const char* name, const char* ipStr, const int port)
    : CRunnableTenant(name), udp(CIPv4(ipStr), port, port), port(port) {}

void COrkToggleTenant::Run() {
    uint8_t trigger = 0;
    const int ret = udp.ReceiveAsynchronous(&trigger, sizeof(trigger));
    if (ret <= 0) {
        return;
    }

    const int err = ork_trigger_launch();
    if (err == 0) {
        LOG_INF("Received OpenRocket launch trigger on UDP port %d", port);
    } else if (err == -EALREADY) {
        LOG_WRN("Ignoring duplicate OpenRocket launch trigger on UDP port %d", port);
    } else {
        LOG_ERR("Failed to trigger OpenRocket launch (%d)", err);
    }
}
