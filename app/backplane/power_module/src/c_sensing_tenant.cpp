#include "c_sensing_tenant.h"
#include "c_power_module.h"

#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_gyroscope.h>
#include <f_core/device/sensor/c_magnetometer.h>
#include <f_core/device/sensor/c_temperature_sensor.h>
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
