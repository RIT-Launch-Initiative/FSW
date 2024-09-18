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
    // TODO: Zero out when we have simulated shunt
    CPowerModule::SensorData data{
        .RailBattery = {
            .Voltage = 12.0f,
            .Current = 0.0f,
            .Power = 0.0f
        },
        .Rail3v3 = {
            .Voltage = 3.3f,
            .Current = 0.0f,
            .Power = 0.0f
        },
        .Rail5v0 = {
            .Voltage = 5.0f,
            .Current = 0.0f,
            .Power = 0.0f
        }
    };

#ifndef CONFIG_ARCH_POSIX
    CShunt shuntBatt(*DEVICE_DT_GET(DT_ALIAS(shunt_batt)));
    CShunt shunt3v3(*DEVICE_DT_GET(DT_ALIAS(shunt_3v3)));
    CShunt shunt5v0(*DEVICE_DT_GET(DT_ALIAS(shunt_5v0)));
    CSensorDevice *sensors[] = {&shuntBatt, &shunt3v3, &shunt5v0};
#endif

    while (true) {
#ifndef CONFIG_ARCH_POSIX
        for (auto sensor: sensors) {
            sensor->UpdateSensorValue();
        }

        data.Rail3v3.Current = shunt3v3.GetSensorValueFloat(SENSOR_CHAN_CURRENT);
        data.Rail3v3.Voltage = shunt3v3.GetSensorValueFloat(SENSOR_CHAN_VOLTAGE);
        data.Rail3v3.Power = shunt3v3.GetSensorValueFloat(SENSOR_CHAN_POWER);

        data.Rail5v0.Current = shunt5v0.GetSensorValueFloat(SENSOR_CHAN_CURRENT);
        data.Rail5v0.Voltage = shunt5v0.GetSensorValueFloat(SENSOR_CHAN_VOLTAGE);
        data.Rail5v0.Power = shunt5v0.GetSensorValueFloat(SENSOR_CHAN_POWER);

        data.Rail5v0.Current = shunt5v0.GetSensorValueFloat(SENSOR_CHAN_CURRENT);
        data.Rail5v0.Voltage = shunt5v0.GetSensorValueFloat(SENSOR_CHAN_VOLTAGE);
        data.Rail5v0.Power = shunt5v0.GetSensorValueFloat(SENSOR_CHAN_POWER);
#endif


        k_msgq_put(&broadcastQueue, &data, K_NO_WAIT);
        k_msleep(100);
    }
}