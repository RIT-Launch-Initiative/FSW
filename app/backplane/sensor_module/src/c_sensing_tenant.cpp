#include "c_sensing_tenant.h"
#include "c_sensor_module.h"

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
    // IMU
    CAccelerometer imuAccelerometer(*DEVICE_DT_GET(DT_ALIAS(imu)));
    CGyroscope imuGyroscope(*DEVICE_DT_GET(DT_ALIAS(imu)));

    // Barometers
    CBarometer primaryBarometer(*DEVICE_DT_GET(DT_ALIAS(primary_barometer)));
    CBarometer secondaryBarometer(*DEVICE_DT_GET(DT_ALIAS(secondary_barometer)));

    // Individual
    CAccelerometer accelerometer(*DEVICE_DT_GET(DT_ALIAS(accelerometer)));
    CMagnetometer magnetometer(*DEVICE_DT_GET(DT_ALIAS(magnetometer)));
    CTemperatureSensor thermometer(*DEVICE_DT_GET(DT_ALIAS(thermometer)));

    CSensorDevice* sensors[] = {&imuAccelerometer, &imuGyroscope, &primaryBarometer, &secondaryBarometer,
                                &accelerometer, &magnetometer, &thermometer};

    CSensorModule::SensorData data{};
    while (true) {
        for (auto sensor : sensors) {
            sensor->UpdateSensorValue();
        }

        data.Acceleration.X = accelerometer.GetSensorValueFloat(SENSOR_CHAN_GYRO_X);
        data.Acceleration.Y = accelerometer.GetSensorValueFloat(SENSOR_CHAN_GYRO_Y);
        data.Acceleration.Z = accelerometer.GetSensorValueFloat(SENSOR_CHAN_GYRO_Z);

        data.ImuAcceleration.X = imuAccelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_X);
        data.ImuAcceleration.Y = imuAccelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_Y);
        data.ImuAcceleration.Z = imuAccelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_Z);

        data.ImuGyroscope.X = imuGyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_X);
        data.ImuGyroscope.Y = imuGyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_Y);
        data.ImuGyroscope.Z = imuGyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_Z);

        data.Magnetometer.X = magnetometer.GetSensorValueFloat(SENSOR_CHAN_GYRO_X);
        data.Magnetometer.Y = magnetometer.GetSensorValueFloat(SENSOR_CHAN_GYRO_Y);
        data.Magnetometer.Z = magnetometer.GetSensorValueFloat(SENSOR_CHAN_GYRO_Z);

        data.PrimaryBarometer.Pressure = primaryBarometer.GetSensorValueFloat(SENSOR_CHAN_PRESS);
        data.PrimaryBarometer.Temperature = primaryBarometer.GetSensorValueFloat(SENSOR_CHAN_AMBIENT_TEMP);

        data.SecondaryBarometer.Pressure = secondaryBarometer.GetSensorValueFloat(SENSOR_CHAN_PRESS);
        data.SecondaryBarometer.Temperature = secondaryBarometer.GetSensorValueFloat(SENSOR_CHAN_AMBIENT_TEMP);

        data.Temperature = thermometer.GetSensorValueFloat(SENSOR_CHAN_AMBIENT_TEMP);

        k_msgq_put(&broadcastQueue, &data, K_NO_WAIT);
        k_msleep(100);
    }
}
