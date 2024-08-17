#include "c_sensing_tenant.h"
#include "c_sensor_module.h"

#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_gyroscope.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CSensingTenant);

extern k_msgq broadcastQueue;

void CSensingTenant::Startup() {

}

void CSensingTenant::PostStartup() {

}

void CSensingTenant::Run() {
    CAccelerometer imu_accelerometer(*DEVICE_DT_GET(DT_ALIAS(imu)));
    CGyroscope imu_gyroscope(*DEVICE_DT_GET(DT_ALIAS(imu)));
    CBarometer barometer(*DEVICE_DT_GET(DT_ALIAS(primary_barometer)));

    CSensorDevice* sensors[] = {&imu_accelerometer, &imu_gyroscope, &barometer};

    CSensorModule::SensorData data{};
    while (true) {
        for (auto sensor : sensors) {
            sensor->UpdateSensorValue();
        }

        data.AccelerationX = imu_accelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_X);
        data.AccelerationY = imu_accelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_Y);
        data.AccelerationZ = imu_accelerometer.GetSensorValueFloat(SENSOR_CHAN_ACCEL_Z);

        data.GyroscopeX = imu_gyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_X);
        data.GyroscopeY = imu_gyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_Y);
        data.GyroscopeZ = imu_gyroscope.GetSensorValueFloat(SENSOR_CHAN_GYRO_Z);

        data.Pressure = barometer.GetSensorValueFloat(SENSOR_CHAN_PRESS);
        data.Temperature = barometer.GetSensorValueFloat(SENSOR_CHAN_AMBIENT_TEMP);

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
