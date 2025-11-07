#include "c_sensing_tenant.h"

#include "c_sensor_module.h"

#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_gyroscope.h>
#include <f_core/device/sensor/c_magnetometer.h>
#include <f_core/device/sensor/c_temperature_sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CSensingTenant);

CSensingTenant::CSensingTenant(const char* name, CMessagePort<NTypes::SensorData>& dataToBroadcast,
                               CMessagePort<NTypes::LoRaBroadcastSensorData>& downlinkDataToBroadcast,
                               CMessagePort<NTypes::TimestampedSensorData>& dataToLog, CDetectionHandler& handler)
    : CRunnableTenant(name), dataToBroadcast(dataToBroadcast), dataToLog(dataToLog),
      dataToDownlink(downlinkDataToBroadcast), detectionHandler(handler),
      imuAccelerometer(*DEVICE_DT_GET(DT_ALIAS(imu))), imuGyroscope(*DEVICE_DT_GET(DT_ALIAS(imu))),
      primaryBarometer(*DEVICE_DT_GET(DT_ALIAS(primary_barometer))),
      secondaryBarometer(*DEVICE_DT_GET(DT_ALIAS(secondary_barometer))),
      accelerometer(*DEVICE_DT_GET(DT_ALIAS(accelerometer))), thermometer(*DEVICE_DT_GET(DT_ALIAS(thermometer))),
      magnetometer(*DEVICE_DT_GET(DT_ALIAS(magnetometer))),
      sensors{&imuAccelerometer, &imuGyroscope, &primaryBarometer, &secondaryBarometer, &accelerometer, &thermometer,
#ifndef CONFIG_ARCH_POSIX
              &magnetometer
#endif
      } {
}

void CSensingTenant::Startup() {
    sendingTimer.StartTimer(10 * 1000);
    broadcastTimer.StartTimer(100);

#ifndef CONFIG_ARCH_POSIX
    const sensor_value imuOdr{.val1 = 104, .val2 = 0};

    if (imuAccelerometer.Configure(SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &imuOdr)) {
        LOG_WRN("IMU Accelerometer ODR configuration failed. IMU accelerations will report 0.");
    }

    if (imuGyroscope.Configure(SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &imuOdr)) {
        LOG_WRN("IMU Gyroscope ODR configuration failed. IMU gyroscope values will report 0.");
    }

#endif
}

void CSensingTenant::PostStartup() {}

void CSensingTenant::Run() {
    NTypes::TimestampedSensorData timestampedData{.timestamp = 0, .data = {0}};
    NTypes::SensorData& data = timestampedData.data;

    uint64_t uptime = k_uptime_get();

    CDetectionHandler::SensorWorkings sensor_states = {};
    imuGyroscope.UpdateSensorValue();
    sensor_states.primaryAccOk = imuAccelerometer.UpdateSensorValue();
    sensor_states.primaryBarometerOk = primaryBarometer.UpdateSensorValue();
    sensor_states.secondaryBarometerOk = secondaryBarometer.UpdateSensorValue();
    sensor_states.secondaryAccOk = accelerometer.UpdateSensorValue();
    thermometer.UpdateSensorValue();
#ifndef CONFIG_ARCH_POSIX
    magnetometer.UpdateSensorValue();
#endif

    // Note that compilers don't accept references to packed struct fields

    // this clobbers sensor_states for some damn reason
    // uint32_t tmpTimestamp = 0;
    // if (int ret = rtc.GetMillisTime(tmpTimestamp); ret < 0) {
        // LOG_ERR("Failed to get time from RTC");
    timestampedData.timestamp = k_uptime_get();
    // } else {
        // timestampedData.timestamp = tmpTimestamp;
    // }



    data.Acceleration.X = accelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_X);
    data.Acceleration.Y = accelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_Y);
    data.Acceleration.Z = accelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_Z);

    data.ImuAcceleration.X = imuAccelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_X);
    data.ImuAcceleration.Y = imuAccelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_Y);
    data.ImuAcceleration.Z = imuAccelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_Z);

    data.ImuGyroscope.X = imuGyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_X);
    data.ImuGyroscope.Y = imuGyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_Y);
    data.ImuGyroscope.Z = imuGyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_Z);

    data.Magnetometer.X = magnetometer.GetSensorValueFloat(SENSOR_CHAN_MAGN_X);
    data.Magnetometer.Y = magnetometer.GetSensorValueFloat(SENSOR_CHAN_MAGN_Y);
    data.Magnetometer.Z = magnetometer.GetSensorValueFloat(SENSOR_CHAN_MAGN_Z);

    data.PrimaryBarometer.Pressure = primaryBarometer.GetSensorValueFloat(SENSOR_CHAN_PRESS);
    data.PrimaryBarometer.Temperature = primaryBarometer.GetSensorValueFloat(SENSOR_CHAN_AMBIENT_TEMP);

    data.SecondaryBarometer.Pressure = secondaryBarometer.GetSensorValueFloat(SENSOR_CHAN_PRESS);
    data.SecondaryBarometer.Temperature = secondaryBarometer.GetSensorValueFloat(SENSOR_CHAN_AMBIENT_TEMP);

    data.Temperature.Temperature = thermometer.GetSensorValueFloat(SENSOR_CHAN_AMBIENT_TEMP);

    // If we can't send immediately, drop the packet
    // we're gonna sleep then give it new data anywas
    if (broadcastTimer.IsExpired()) {
        if (dataToBroadcast.Send(data, K_NO_WAIT)) {
            LOG_ERR("Failed to send sensor data to broadcast port");
        } else {
            LOG_DBG("Sensor data sent to broadcast port");
        }
    }
    if (sendingTimer.IsExpired()) {
        sendDownlinkData(data);
    }

    detectionHandler.HandleData(uptime, data, sensor_states);
    if (detectionHandler.FlightOccurring()) {
        int ret = dataToLog.Send(timestampedData, K_NO_WAIT);
        if (ret) {
            LOG_ERR("Failed to send sensor data to log port");
        }

        LOG_WRN_ONCE("Beginning logging");
        if (dataToLog.AvailableSpace() < 100) {
            NRtos::ResumeTask("Data Logging Task");
            k_yield();
        }
    }

    // Don't care about performance at this point,
    // Keep making sure the data we sent gets logged
    if (detectionHandler.FlightFinished()) {
        NRtos::ResumeTask("Data Logging Task");
        k_msleep(1000);
    }
}

void CSensingTenant::sendDownlinkData(const NTypes::SensorData& data) {
    NTypes::LoRaBroadcastSensorData downlinkData{
        .Barometer =
            {
                .Pressure = static_cast<int16_t>(data.PrimaryBarometer.Pressure),
                .Temperature = static_cast<int16_t>(data.PrimaryBarometer.Temperature),
            },
        .Acceleration =
            {
                .X = static_cast<int16_t>(CSensorDevice::ToMilliUnits(data.ImuAcceleration.X)),
                .Y = static_cast<int16_t>(CSensorDevice::ToMilliUnits(data.ImuAcceleration.Y)),
                .Z = static_cast<int16_t>(CSensorDevice::ToMilliUnits(data.ImuAcceleration.Z)),
            },
        .Gyroscope =
            {
                .X = static_cast<int16_t>(CSensorDevice::ToMilliUnits(data.ImuGyroscope.X)),
                .Y = static_cast<int16_t>(CSensorDevice::ToMilliUnits(data.ImuGyroscope.Y)),
                .Z = static_cast<int16_t>(CSensorDevice::ToMilliUnits(data.ImuGyroscope.Z)),
            },
        .BootCount = 2,
        .Phase = detectionHandler.PhaseByte(),
    };

    dataToDownlink.Send(downlinkData, K_NO_WAIT);
}

