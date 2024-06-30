#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

const struct device *imu_dev = DEVICE_DT_GET_ONE(openrocket_imu);

int main() {
    if (!device_is_ready(imu_dev)) {
        printk("IMU is not ready\n");
        return -ENODEV;
    }
    while (1) {
        sensor_sample_fetch(imu_dev);
        struct sensor_value vals[3];
        sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, vals);
        double x = sensor_value_to_double(&vals[0]);
        double y = sensor_value_to_double(&vals[1]);
        double z = sensor_value_to_double(&vals[2]);

        printk("(%.2f, %.2f, %.2f)\n", x, y, z);
        k_msleep(100);
    }
    return 0;
}
