#include "c_bcast_rcv_tenant.h"
#include "c_radio_module.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CBroadcastReceiveTenant);

extern k_msgq broadcastQueue;

void CBroadcastReceiveTenant::Startup() {
}

void CBroadcastReceiveTenant::PostStartup() {
}

void CBroadcastReceiveTenant::Run() {
    while (true) {

    }
}
