#include "orient.h"

#include <algorithm>
#include <math.h>
#include <zephyr/drivers/sensor.h>

#define IMU_NODE DT_ALIAS(imu)
static const struct device *imu_dev = DEVICE_DT_GET(IMU_NODE);

int Servo::close() const int Servo::set_pulse(uint32_t pulse)

    int Servo::disconnect() const {
    return pwm_set_pulse_dt(&pwm, 0);
}

bool Servo::was_fully_open() const { return state.last_ticks == open_pulselen; }
bool Servo::was_fully_closed() const { return state.last_ticks == closed_pulselen; }

int find_vector(vec3 &me) {

    int ret = sensor_sample_fetch(imu_dev);
    if (ret < 0) {
        printk("Failed to fetch imu: %d\n", ret);
        return ret;
    }
    struct sensor_value xyz[3] = {0};
    ret = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, &xyz[0]);
    if (ret < 0) {
        printk("Failed to get imu: %d\n", ret);
        return ret;
    }
    me = {sensor_value_to_float(&xyz[0]), sensor_value_to_float(&xyz[1]), sensor_value_to_float(&xyz[2])};
    printk("[%f, %f, %f]", me.x, me.y, me.z);
    float norm = sqrt(me.x * me.x + me.y * me.y + me.z * me.z);
    me.x /= norm;
    me.y /= norm;
    me.z /= norm;

    return 0;
}