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

        data.AccelerationX = imuAccelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_X);
        data.AccelerationY = imuAccelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_Y);
        data.AccelerationZ = imuAccelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_Z);

        data.GyroscopeX = imuGyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_X);
        data.GyroscopeY = imuGyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_Y);
        data.GyroscopeZ = imuGyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_Z);

        data.Pressure = primaryBarometer.GetSensorValueFloat(SENSOR_CHAN_PRESS);
        data.Temperature = primaryBarometer.GetSensorValueFloat(SENSOR_CHAN_AMBIENT_TEMP);

        LOG_INF(
            "\nAccelerationX: %f\nAccelerationY: %f\nAccelerationZ: %f"
            "\nGyroscopeX: %f\nGyroscopeY: %f\nGyroscopeZ: %f"
            "\nPressure: %f\nTemperature: %f\n",
            (double) data.AccelerationX, (double) data.AccelerationY, (double) data.AccelerationZ,
            (double) data.GyroscopeX, (double) data.GyroscopeY, (double) data.GyroscopeZ,
            (double) data.Pressure, (double) data.Temperature);
        k_msgq_put(&broadcastQueue, &data, K_NO_WAIT);
        k_msleep(100);
    }
}
