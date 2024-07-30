#include "c_broadcast_tenant.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CBroadcastTenant);

void CBroadcastTenant::Startup() {
}

void CBroadcastTenant::PostStartup() {
}

void CBroadcastTenant::Run() {
    while (true) {
        udp.TransmitSynchronous("Hello, Launch!", 14);
        LOG_INF("Transmitted");
        k_sleep(K_SECONDS(1));
    }
}
