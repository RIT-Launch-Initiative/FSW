#include "n_storage.h"
#include "servo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// #pragma GCC diagnostic ignored "-Wdeprecated"
#include "quaternion.h"
#pragma GCC diagnostic pop
using namespace quaternion;

#include <array>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_AIRBRAKE_LOG_LEVEL);

SYS_INIT(servo_init, APPLICATION, 1);
SYS_INIT(storage_init, APPLICATION, 2);

#define IMU_NODE DT_NODELABEL(lsm6dv)
const struct device* imu_dev = DEVICE_DT_GET(IMU_NODE);

int gyro_to_euler(std::array<double, 3>& angles) {
    struct sensor_value xyz[3] = {0};
    int ret = sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, xyz);
    if (ret != 0) {
        printk("failed to read gyro: %d\n", ret);
        return ret;
    }
    // printk("data" PRIsensor_three_axis_data "\n", PRIsensor_three_axis_data_arg(xyz, 0))
    angles[0] = sensor_value_to_double(&xyz[0]);
    angles[1] = sensor_value_to_double(&xyz[1]);
    angles[2] = sensor_value_to_double(&xyz[2]);

    return 0;
}

Quaternion<> angular_rate_to_quaternion_rotation(std::array<double, 3> w, double dt) {
    // w is the vector indicating angular rate in the reference frame of the
    // IMU, all coords in rad/s
    // dt is the time interval during which the angular rate is valid

    double wx = w[0];
    double wy = w[1];
    double wz = w[2];

    double l = sqrt(wx * wx + wy * wy + wz * wz);

    double dtlo2 = dt * l / 2;

    double q0 = cos(dtlo2);
    double q1 = sin(dtlo2) * wx / l;
    double q2 = sin(dtlo2) * wy / l;
    double q3 = sin(dtlo2) * wz / l;

    return Quaternion(q0, q1, q2, q3);
}

int main() {

    float target_hz = 1000;
    struct sensor_value target_hz_s = {0};
    sensor_value_from_float(&target_hz_s, target_hz);

    int ret = sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &target_hz_s);
    if (ret != 0) {
        printk("Error setting gyro sampling freq: %d\n", ret);
    }
    ret = sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &target_hz_s);
    if (ret != 0) {
        printk("Error setting acc sampling freq: %d\n", ret);
    }
    k_msleep(1000);

    printk("Calibrating\n");

    double offset[3] = {0};
    const int calib_len = 100;
    for (int i = 0; i < calib_len; i++) {
        std::array<double, 3> euler_angles{2};
        sensor_sample_fetch_chan(imu_dev, SENSOR_CHAN_ACCEL_XYZ);
        sensor_sample_fetch_chan(imu_dev, SENSOR_CHAN_GYRO_XYZ);
        ret = gyro_to_euler(euler_angles);
        printk("%.5f, %.5f, %.5f\n", euler_angles[0], euler_angles[1], euler_angles[2]);
        if (ret != 0) {
            printk("Error reading: %d\n", ret);
            continue;
        }
        offset[0] += euler_angles[0];
        offset[1] += euler_angles[1];
        offset[2] += euler_angles[2];
        k_msleep(10);
    }
    printk("Time to break air\n");

    offset[0] /= calib_len;
    offset[1] /= calib_len;
    offset[2] /= calib_len;
    printk("offsets: %.5f, %.5f, %.5f\n", offset[0], offset[1], offset[2]);

    Quaternion<> orientation{1,0,0,0};

    int64_t last_reading = k_uptime_get();
    size_t frame = 0;
    while (true) {
        // printk("frame; %d\n", frame);
        frame++;

        std::array<double, 3> euler_angles{2};
        int64_t deltaT = k_uptime_delta(&last_reading);
        sensor_sample_fetch_chan(imu_dev, SENSOR_CHAN_ACCEL_XYZ);
        sensor_sample_fetch_chan(imu_dev, SENSOR_CHAN_GYRO_XYZ);
        last_reading = k_uptime_get();
// 
        ret = gyro_to_euler(euler_angles);

        euler_angles[0] -= offset[0];
        euler_angles[1] -= offset[1];
        euler_angles[2] -= offset[2];

        if (ret != 0) {
            printk("Error reading: %d\n", ret);
            continue;
        }
        double dt = (double) deltaT / 1000.0;
        Quaternion<> delta = angular_rate_to_quaternion_rotation(euler_angles, dt);

        orientation = orientation * delta;

        if (frame % 10 == 0) {
            auto norm = normalize(orientation);
            auto vec = to_euler(norm);
            // printk("%.5f, %.5f, %.5f\n", euler_angles[0], euler_angles[1], euler_angles[2]);
            printk("%llu, %.5f, %.5f, %.5f, %.5f\n", last_reading, norm.a(), norm.b(), norm.c(), norm.d());
        }

        k_msleep(10);
    }
}
