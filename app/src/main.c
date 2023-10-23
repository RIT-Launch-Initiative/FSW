/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <app_version.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

static struct sensor_value accel_x_out;
static struct sensor_value accel_y_out;
static struct sensor_value accel_z_out;
static struct sensor_value gyro_x_out;
static struct sensor_value gyro_y_out;
static struct sensor_value gyro_z_out;

static int print_samples;
static int lsm6dsl_trig_cnt;

static inline float out_ev(struct sensor_value *val) {
	return (val->val1 + (float) val->val2 / 1000000);
}


static void lsm6dsl_trigger_handler(const struct device *dev, const struct sensor_trigger *trig) {
    static struct sensor_value accel_x, accel_y, accel_z;
    static struct sensor_value gyro_x, gyro_y, gyro_z;

    lsm6dsl_trig_cnt++;

    sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
    sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &accel_x);
    sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &accel_y);
    sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &accel_z);

    sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
    sensor_channel_get(dev, SENSOR_CHAN_GYRO_X, &gyro_x);
    sensor_channel_get(dev, SENSOR_CHAN_GYRO_Y, &gyro_y);
    sensor_channel_get(dev, SENSOR_CHAN_GYRO_Z, &gyro_z);

    if (print_samples) {
        print_samples = 0;

        accel_x_out = accel_x;
        accel_y_out = accel_y;
        accel_z_out = accel_z;

        gyro_x_out = gyro_x;
        gyro_y_out = gyro_y;
        gyro_z_out = gyro_z;
    }
}

int main(void) {
    printk("Zephyr Example Application %s\n", APP_VERSION_STRING);

    while (1) {
        printk("Launch!\n");
    }

    return 0;
}

