#ifndef C_SENSING_TENANT_H
#define C_SENSING_TENANT_H

#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>
#include <zephyr/device.h>
#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_gyroscope.h>
#include <f_core/device/sensor/c_magnetometer.h>
#include <f_core/device/sensor/c_temperature_sensor.h>
#include <array>
#include "n_types.h"

class CSensorDevice;

class CSensingTenant : public CTenant {
public:
    explicit CSensingTenant(const char* name, CMessagePort<NTypes::SensorData> &dataToBroadcast, CMessagePort<NTypes::SensorData> &dataToLog);
    ~CSensingTenant() override = default;

    void Startup() override;
    void PostStartup() override;
    void Run() override;

private:
    CMessagePort<NTypes::SensorData> &dataToBroadcast;
    CMessagePort<NTypes::SensorData> &dataToLog;

    // Sensor instances
    CAccelerometer imuAccelerometer;
    CGyroscope imuGyroscope;
    CBarometer primaryBarometer;
    CBarometer secondaryBarometer;
    CAccelerometer accelerometer;
    CTemperatureSensor thermometer;

#ifndef CONFIG_ARCH_POSIX
    CMagnetometer magnetometer;
#endif

    std::array<CSensorDevice*, 7> sensors;
};

#endif // C_SENSING_TENANT_H
