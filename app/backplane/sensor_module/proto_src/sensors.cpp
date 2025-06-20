#include <n_autocoder_types.h>

#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_gyroscope.h>
#include <f_core/device/sensor/c_magnetometer.h>
#include <f_core/device/sensor/c_temperature_sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(CSensingTenant);

ZBUS_CHAN_DEFINE(sensor_data_chan,     // Name
                 NTypes::SensorData,   // Type
                 NULL,                 // Validator
                 NULL,                 // Observer notification callback
                 ZBUS_OBSERVERS_EMPTY, // Initial observers
                 {0}                   // Initial value
);

static CAccelerometer imuAccelerometer(*DEVICE_DT_GET(DT_ALIAS(imu)));
static CGyroscope imuGyroscope(*DEVICE_DT_GET(DT_ALIAS(imu)));
static CBarometer primaryBarometer(*DEVICE_DT_GET(DT_ALIAS(primary_barometer)));
static CBarometer secondaryBarometer(*DEVICE_DT_GET(DT_ALIAS(secondary_barometer)));
static CAccelerometer accelerometer(*DEVICE_DT_GET(DT_ALIAS(accelerometer)));
static CTemperatureSensor thermometer(*DEVICE_DT_GET(DT_ALIAS(thermometer)));
static CMagnetometer magnetometer(*DEVICE_DT_GET(DT_ALIAS(magnetometer)));

void sensorsInit() {
    const sensor_value imuOdr{.val1 = 104, .val2 = 0};

    if (imuAccelerometer.Configure(SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &imuOdr)) {
        LOG_WRN("IMU Accelerometer ODR configuration failed. IMU accelerations will report 0.");
    }

    if (imuGyroscope.Configure(SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &imuOdr)) {
        LOG_WRN("IMU Gyroscope ODR configuration failed. IMU gyroscope values will report 0.");
    }
}

void readSensors() {
    NTypes::TimestampedSensorData timestampedData{
        .timestamp = 0,
        .data = {0}
    };
    NTypes::SensorData& data = timestampedData.data;

    imuGyroscope.UpdateSensorValue();
    thermometer.UpdateSensorValue();
#ifndef CONFIG_ARCH_POSIX
    magnetometer.UpdateSensorValue();
#endif
    int64_t start = k_uptime_get();


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

    if (int ret = zbus_chan_pub(&sensor_data_chan, &data, K_MSEC(100))) {
        LOG_ERR("Failed to publish sensor data to zbus: %d", ret);
    } else {
        LOG_INF("Sensor data published successfully");
    }

    int64_t delta = k_uptime_delta(&start);
    if (delta < 10) {
        LOG_INF("Delta was %lld ms", delta);
        k_msleep(10 - delta);
    }
}
