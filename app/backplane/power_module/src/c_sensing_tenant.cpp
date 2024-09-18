#include "c_sensing_tenant.h"
#include "c_power_module.h"

#include <f_core/device/sensor/c_shunt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CSensingTenant);

extern k_msgq broadcastQueue;

void CSensingTenant::Startup() {
}

void CSensingTenant::PostStartup() {
}

void CSensingTenant::Run() {
    CPowerModule::SensorData data{};
    while (true) {
        // for (auto sensor: sensors) {
        //     sensor->UpdateSensorValue();
        // }

        k_msgq_put(&broadcastQueue, &data, K_NO_WAIT);
        k_msleep(100);
    }
}
