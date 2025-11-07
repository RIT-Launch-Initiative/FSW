#ifndef C_SENSING_TENANT_H
#define C_SENSING_TENANT_H

#include "c_detection_handler.h"
#include "f_core/device/c_rtc.h"

#include <array>
#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_gyroscope.h>
#include <f_core/device/sensor/c_magnetometer.h>
#include <f_core/device/sensor/c_temperature_sensor.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_runnable_tenant.h>
#include <n_autocoder_types.h>
#include <zephyr/device.h>
#include <f_core/utils/c_soft_timer.h>

class CSensorDevice;

class CSensingTenant : public CRunnableTenant {
  public:
    explicit CSensingTenant(const char *name, CMessagePort<NTypes::SensorData> &dataToBroadcast, CMessagePort<NTypes::LoRaBroadcastSensorData> &downlinkDataToBroadcast,
                            CMessagePort<NTypes::TimestampedSensorData> &dataToLog, CDetectionHandler &handler);
    ~CSensingTenant() override = default;

    void Startup() override;
    void PostStartup() override;
    void Run() override;

  private:
    CMessagePort<NTypes::SensorData> &dataToBroadcast;
    CMessagePort<NTypes::TimestampedSensorData> &dataToLog;
    CMessagePort<NTypes::LoRaBroadcastSensorData> &dataToDownlink;

    CDetectionHandler &detectionHandler;
    // Sensor instances
    CAccelerometer imuAccelerometer;
    CGyroscope imuGyroscope;
    CBarometer primaryBarometer;
    CBarometer secondaryBarometer;
    CAccelerometer accelerometer;
    CTemperatureSensor thermometer;
    CMagnetometer magnetometer;
    CSoftTimer sendingTimer{nullptr, nullptr};
    CSoftTimer broadcastTimer{nullptr, nullptr};
    CSoftTimer sensingTimer{nullptr, nullptr};
    std::array<CSensorDevice *, 7> sensors;

    CRtc rtc{*DEVICE_DT_GET(DT_ALIAS(rtc))};

    void sendDownlinkData(const NTypes::SensorData &data);
};

#endif // C_SENSING_TENANT_H
