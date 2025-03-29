#ifndef C_SENSING_TENANT_H
#define C_SENSING_TENANT_H

#include "c_detection_handler.h"

#include <array>
#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_gyroscope.h>
#include <f_core/device/sensor/c_magnetometer.h>
#include <f_core/device/sensor/c_temperature_sensor.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>
#include <n_autocoder_types.h>
#include <f_core/utils/c_observer.h>
#include <zephyr/device.h>

class CSensorDevice;

class CSensingTenant : public CTenant {
  public:
    explicit CSensingTenant(const char *name, CMessagePort<NTypes::SensorData> &dataToBroadcast,
                            CMessagePort<NTypes::SensorData> &dataToLog, CDetectionHandler &handler);
    ~CSensingTenant() override = default;

    void Startup() override;
    void PostStartup() override;
    void Run() override;

  private:
    CMessagePort<NTypes::SensorData> &dataToBroadcast;
    CMessagePort<NTypes::SensorData> &dataToLog;

    CDetectionHandler &detectionHandler;
    // Sensor instances
    CAccelerometer imuAccelerometer;
    CGyroscope imuGyroscope;
    CBarometer primaryBarometer;
    CBarometer secondaryBarometer;
    CAccelerometer accelerometer;
    CTemperatureSensor thermometer;
    CMagnetometer magnetometer;

    std::array<CSensorDevice *, 7> sensors;
};

#endif // C_SENSING_TENANT_H
