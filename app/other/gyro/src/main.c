/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdalign.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

static int lsm6dsl_trig_cnt;

struct samp {
    uint64_t ts; // ms
    double ax, ay, az;
    double gx, gy, gz;
};
K_MSGQ_DEFINE(samples, sizeof(struct samp), 10, alignof(struct samp));

static void lsm6dsl_trigger_handler(const struct device *dev, const struct sensor_trigger *trig) {
    static struct sensor_value accel_x, accel_y, accel_z;
    static struct sensor_value gyro_x, gyro_y, gyro_z;

    lsm6dsl_trig_cnt++;
    sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
    uint64_t ts = k_ticks_to_us_near64(k_uptime_ticks()) ;
    sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &accel_x);
    sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &accel_y);
    sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &accel_z);

    /* lsm6dsl gyro */
    sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
    sensor_channel_get(dev, SENSOR_CHAN_GYRO_X, &gyro_x);
    sensor_channel_get(dev, SENSOR_CHAN_GYRO_Y, &gyro_y);
    sensor_channel_get(dev, SENSOR_CHAN_GYRO_Z, &gyro_z);

    static struct samp s = {0};
    s.ts = ts;
    s.ax = sensor_value_to_double(&accel_x);
    s.az = sensor_value_to_double(&accel_y);
    s.az = sensor_value_to_double(&accel_z);

    s.gx = sensor_value_to_double(&gyro_x);
    s.gy = sensor_value_to_double(&gyro_y);
    s.gz = sensor_value_to_double(&gyro_z);

    k_msgq_put(&samples, &s, K_NO_WAIT);
}

int main(void) {
    k_msleep(2000);
    struct sensor_value odr_attr;
    const struct device *const lsm6dsl_dev = DEVICE_DT_GET_ONE(st_lsm6dsv16x);

    if (!device_is_ready(lsm6dsl_dev)) {
        printk("sensor: device not ready.\n");
        return 0;
    }

    /* set accel/gyro sampling frequency to 104 Hz */
    odr_attr.val1 = 104;
    odr_attr.val2 = 0;

    if (sensor_attr_set(lsm6dsl_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
        printk("Cannot set sampling frequency for accelerometer.\n");
        return 0;
    }

    if (sensor_attr_set(lsm6dsl_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
        printk("Cannot set sampling frequency for gyro.\n");
        return 0;
    }

    struct sensor_value accel_fs;
    sensor_g_to_ms2(16, &accel_fs);
    struct sensor_value gyro_fs;
    sensor_value_from_float(&gyro_fs, 34.906585);

    if (sensor_attr_set(lsm6dsl_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &accel_fs) < 0) {
        printk("Cannot set FS Range for accel.\n");
        return 0;
    }

    if (sensor_attr_set(lsm6dsl_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE, &gyro_fs) < 0) {
        printk("Cannot set sampling frequency for gyro.\n");
        return 0;
    }

    struct sensor_trigger trig;

    trig.type = SENSOR_TRIG_DATA_READY;
    trig.chan = SENSOR_CHAN_ACCEL_XYZ;

    if (sensor_trigger_set(lsm6dsl_dev, &trig, lsm6dsl_trigger_handler) != 0) {
        printk("Could not set sensor type and channel\n");
        return 0;
    }

    if (sensor_sample_fetch(lsm6dsl_dev) < 0) {
        printk("Sensor sample update error\n");
        return 0;
    }

    uint64_t last_us = 0;
    while (1) {
        /* Erase previous */
        printf("LSM6DSL sensor samples:\n\n");
        printk("ms, dt, ax, ay, az, gx, gy, gz");
        static struct samp s = {0};
        while (true) {
            int ret = k_msgq_get(&samples, &s, K_FOREVER);
            if (ret != 0) {
                printk("ERROR WAITING: %d\n", ret);
                continue;
            }

            printk("%llu %llu %+f %+f %+f %+f %+f %+f\n", s.ts, s.ts - last_us, s.ax, s.ay, s.az, s.gx, s.gy, s.gz);
            last_us = s.ts;
        }
    }
}
