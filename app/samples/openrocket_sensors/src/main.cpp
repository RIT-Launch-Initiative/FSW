// F-Core Includes
#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_gyroscope.h>
#include <f_core/device/sensor/c_magnetometer.h>
#include <f_core/device/sensor/c_shunt.h>

// Zephyr Includes
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

int main() {
    CAccelerometer imu_accelerometer(*DEVICE_DT_GET_ONE(openrocket_imu));
    CGyroscope imu_gyroscope(*DEVICE_DT_GET_ONE(openrocket_imu));
    CBarometer barometer(*DEVICE_DT_GET_ONE(openrocket_barometer));
    CMagnetometer magnetometer(*DEVICE_DT_GET_ONE(openrocket_magnetometer));
    CShunt shunt(*DEVICE_DT_GET_ONE(sim_simshunt));

    CSensorDevice *sensors[] = {&imu_accelerometer, &imu_gyroscope, &barometer, &magnetometer, &shunt};

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

        double mx = magnetometer.GetSensorValueDouble(SENSOR_CHAN_MAGN_X);
        double my = magnetometer.GetSensorValueDouble(SENSOR_CHAN_MAGN_Y);
        double mz = magnetometer.GetSensorValueDouble(SENSOR_CHAN_MAGN_Z);

        double volts = shunt.GetSensorValueDouble(SENSOR_CHAN_VOLTAGE);
        double amps = shunt.GetSensorValueDouble(SENSOR_CHAN_CURRENT);

        printk("accel: (%.2f, %.2f, %.2f) - gyro: (%.2f, %.2f, %.2f) - magn(%.2f, %.2f, %.2f)\n", x, y, z, rx, ry, rz,
               mx, my, mz);
        printk("temp: %.2f - press: %.2f\n", temp, press);
        printk("amps: %.2f - volts: %.2f\n", amps, volts);
        k_msleep(1000);
    }

    return 0;
}
