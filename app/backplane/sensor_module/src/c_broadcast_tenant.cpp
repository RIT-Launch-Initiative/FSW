#include "c_broadcast_tenant.h"

#include <c_sensor_module.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CBroadcastTenant);

extern k_msgq broadcastQueue;

void CBroadcastTenant::Startup() {
}

void CBroadcastTenant::PostStartup() {
}

void CBroadcastTenant::Run() {
    while (true) {
        CSensorModule::SensorData data{};
        if (k_msgq_get(&broadcastQueue, &data, K_FOREVER) == 0) {
            LOG_INF("Transmitting %d bytes of data", sizeof(CSensorModule::SensorData));
            udp.TransmitSynchronous(&data, sizeof(CSensorModule::SensorData));
        }
    }
}
