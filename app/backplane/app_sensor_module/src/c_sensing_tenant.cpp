#include "c_sensing_tenant.h"

#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_gyroscope.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CSensingTenant);


void CSensingTenant::Startup() {

}

void CSensingTenant::PostStartup() {

}

void CSensingTenant::Run() {
    CAccelerometer imu_accelerometer(*DEVICE_DT_GET_ONE(openrocket_imu));
    CGyroscope imu_gyroscope(*DEVICE_DT_GET_ONE(openrocket_imu));
    CBarometer barometer(*DEVICE_DT_GET_ONE(openrocket_barometer));

    CSensorDevice *sensors[] = {&imu_accelerometer, &imu_gyroscope, &barometer};

    while (1) {
        for (auto sensor : sensors) {
            sensor->UpdateSensorValue();
        }

        double x = imu_accelerometer.GetSensorValueDouble(SENSOR_CHAN_ACCEL_X);
        double y = imu_accelerometer.GetSensorValueDouble(SENSOR_CHAN_ACCEL_Y);
        double z = imu_accelerometer.GetSensorValueDouble(SENSOR_CHAN_ACCEL_Z);

        double rx = imu_gyroscope.GetSensorValueDouble(SENSOR_CHAN_GYRO_X);
        double ry = imu_gyroscope.GetSensorValueDouble(SENSOR_CHAN_GYRO_Y);
        double rz = imu_gyroscope.GetSensorValueDouble(SENSOR_CHAN_GYRO_Z);

        double press = barometer.GetSensorValueDouble(SENSOR_CHAN_PRESS);
        double temp = barometer.GetSensorValueDouble(SENSOR_CHAN_AMBIENT_TEMP);

        LOG_INF("\nACCEL_X: %.2f\nACCEL_Y: %.2f\nACCEL_Z: %.2f\nGYRO_X: %.2f\nGYRO_Y: %.2f\nGYRO_Z: %.2f\nPRESS: %.2f\nTEMP: %.2f\n", x, y, z, rx, ry, rz, press, temp);
        k_msleep(1000);
    }
}
