#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_gyroscope.h>

int main() {
    CAccelerometer imu_accelerometer(*DEVICE_DT_GET_ONE(openrocket_imu));
    CGyroscope imu_gyroscope(*DEVICE_DT_GET_ONE(openrocket_imu));
    CBarometer barometer(*DEVICE_DT_GET_ONE(openrocket_barometer));


    while (1) {
        imu_accelerometer.UpdateSensorValue();
        imu_gyroscope.UpdateSensorValue();
        barometer.UpdateSensorValue();

        double x = imu_accelerometer.GetSensorValueDouble(SENSOR_CHAN_ACCEL_X);
        double y = imu_accelerometer.GetSensorValueDouble(SENSOR_CHAN_ACCEL_Y);
        double z = imu_accelerometer.GetSensorValueDouble(SENSOR_CHAN_ACCEL_Z);

        double rx = imu_gyroscope.GetSensorValueDouble(SENSOR_CHAN_GYRO_X);
        double ry = imu_gyroscope.GetSensorValueDouble(SENSOR_CHAN_GYRO_Y);
        double rz = imu_gyroscope.GetSensorValueDouble(SENSOR_CHAN_GYRO_Z);

        double press = barometer.GetSensorValueDouble(SENSOR_CHAN_PRESS);
        double temp = barometer.GetSensorValueDouble(SENSOR_CHAN_AMBIENT_TEMP);

        printk("accel: (%.2f, %.2f, %.2f) - gyro: (%.2f, %.2f, %.2f)\n", x, y, z, rx, ry, rz);
        printk("temp: %.2f - press: %.2f\n", temp, press);
        k_msleep(1000);
    }
    return 0;
}
