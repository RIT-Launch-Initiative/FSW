#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_magnetometer.h>
#include <f_core/device/sensor/c_temperature_sensor.h>

int main() {
    const struct device *imu_dev = DEVICE_DT_GET_ONE(openrocket_imu);
    const struct device *barom_dev = DEVICE_DT_GET_ONE(openrocket_barometer);

    CAccelerometer imu_accelerometer(*DEVICE_DT_GET_ONE(openrocket_imu));
    CBarometer barometer(*DEVICE_DT_GET_ONE(openrocket_barometer));


    if (!device_is_ready(imu_dev)) {
        printk("IMU is not ready\n");
        return -ENODEV;
    }
    if (!device_is_ready(barom_dev)) {
        printk("Barometer is not ready\n");
        return -ENODEV;
    }
    while (1) {
        sensor_sample_fetch(imu_dev);
        struct sensor_value vals[3];
        imu_accelerometer.UpdateSensorValue();
        double x = imu_accelerometer.GetSensorValueDouble(SENSOR_CHAN_ACCEL_X);
        double y = imu_accelerometer.GetSensorValueDouble(SENSOR_CHAN_ACCEL_Y);
        double z = imu_accelerometer.GetSensorValueDouble(SENSOR_CHAN_ACCEL_Z);

        sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, vals);
        double rx = sensor_value_to_double(&vals[0]);
        double ry = sensor_value_to_double(&vals[1]);
        double rz = sensor_value_to_double(&vals[2]);

        printk("accel: (%.2f, %.2f, %.2f) - gyro: (%.2f, %.2f, %.2f)\n", x, y, z, rx, ry, rz);

        barometer.UpdateSensorValue();
        double press = barometer.GetSensorValueDouble(SENSOR_CHAN_PRESS);
        double temp = barometer.GetSensorValueDouble(SENSOR_CHAN_AMBIENT_TEMP);
        printk("temp: %.2f - press: %.2f\n", temp, press);
        k_msleep(1000);
    }
    return 0;
}
