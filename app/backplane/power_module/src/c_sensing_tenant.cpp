#include "c_sensing_tenant.h"
#include "c_power_module.h"

#include <f_core/n_alerts.h>
#include <f_core/device/sensor/c_shunt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CSensingTenant);

void CSensingTenant::Startup() {
    static constexpr uint32_t minuteInMillis = 1000 * 60;
    timer.StartTimer(minuteInMillis, 0); // Log every minute on the pad
}

void CSensingTenant::PostStartup() {}

void CSensingTenant::Run() {
    NTypes::SensorData data{
        .RailBattery = {
            .Voltage = 0.0f,
            .Current = 0.0f,
            .Power = 0.0f
        },
        .Rail3v3 = {
            .Voltage = 0.0f,
            .Current = 0.0f,
            .Power = 0.0f
        },
        .Rail5v0 = {
            .Voltage = 0.0f,
            .Current = 0.0f,
            .Power = 0.0f
        }
    };

#ifndef CONFIG_ARCH_POSIX
    CShunt shuntBatt(*DEVICE_DT_GET(DT_ALIAS(shunt_batt)));
    CShunt shunt3v3(*DEVICE_DT_GET(DT_ALIAS(shunt_3v3)));
    CShunt shunt5v0(*DEVICE_DT_GET(DT_ALIAS(shunt_5v0)));
    CSensorDevice* sensors[] = {&shuntBatt, &shunt3v3, &shunt5v0};
#endif

#ifndef CONFIG_ARCH_POSIX
    int i = 0;
    for (auto sensor : sensors) {
        if (!sensor->UpdateSensorValue()) {
            LOG_ERR("Failed to update sensor value for %d", i);
        }
        i++;
    }

    // NOTE: The below calibration values were determined based on using a load tester
    // Zephyr does not support inputting floats into the LSB microamp offset, so we must do it manually

    // Power Module is a black box. Factor it out. We could not have LEDs on for example.
    static constexpr float extra3VCalibrationFactor = 1.13f;
    data.Rail3v3.Current = shunt3v3.GetSensorValueFloat(SENSOR_CHAN_CURRENT) / extra3VCalibrationFactor;
    data.Rail3v3.Voltage = shunt3v3.GetSensorValueFloat(SENSOR_CHAN_VOLTAGE);
    data.Rail3v3.Power = data.Rail3v3.Current * data.Rail3v3.Voltage;

    static constexpr float extra5VCalibration = 0.98f;
    data.Rail5v0.Current = shunt5v0.GetSensorValueFloat(SENSOR_CHAN_CURRENT) * extra5VCalibration;
    data.Rail5v0.Voltage = shunt5v0.GetSensorValueFloat(SENSOR_CHAN_VOLTAGE);
    data.Rail5v0.Power = data.Rail5v0.Current * data.Rail5v0.Voltage;

    static constexpr float extraBattCalibrationDivisor = 1.04f;
    data.RailBattery.Current = shuntBatt.GetSensorValueFloat(SENSOR_CHAN_CURRENT) / extraBattCalibrationDivisor;
    data.RailBattery.Voltage = shuntBatt.GetSensorValueFloat(SENSOR_CHAN_VOLTAGE);
    data.RailBattery.Power = data.RailBattery.Current * data.RailBattery.Voltage;
#endif

    dataToBroadcast.Send(data, K_MSEC(5));

    if (timer.IsExpired()) {
        dataToLog.Send(data, K_MSEC(5));
    }
}

void CSensingTenant::Notify(void* ctx) {
    // TODO: Knock, knock! Race condition...
    switch (*static_cast<NAlerts::AlertType*>(ctx)) {
        case NAlerts::BOOST:
            LOG_INF("Boost detected. Logging data.");
            timer.StartTimer(500); // Log every half a second during flight
            break;

        case NAlerts::LANDED:
            LOG_INF("Landing detected. Disabling logging.");
            timer.StopTimer(); // Stop logging
            break;

        default:
            return;
    }
}
