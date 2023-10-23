/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <app_version.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);



static int print_samples;
static int lsm6dsl_trig_cnt;

static void adxl375_processing_callback(int result, uint8_t *buf,uint32_t buf_len, void *userdata) {
    static struct sensor_value accel_x;
    static struct sensor_value accel_y;
    static struct sensor_value accel_z;

    while (true) {

    }
}

static void bmp388_processing_callback(int result, uint8_t *buf,uint32_t buf_len, void *userdata) {
    static struct sensor_value pressure;
    static struct sensor_value temperature;

    while (true) {

    }
}

static void lsm6dsl_processing_callback(int result, uint8_t *buf,uint32_t buf_len, void *userdata) {
    static struct sensor_value accel_x;
    static struct sensor_value accel_y;
    static struct sensor_value accel_z;
    static struct sensor_value gyro_x;
    static struct sensor_value gyro_y;
    static struct sensor_value gyro_z;

    while (true) {

    }
}

static void lis3mdl_processing_callback(int result, uint8_t *buf,uint32_t buf_len, void *userdata) {
    static struct sensor_value mag_x;
    static struct sensor_value mag_y;
    static struct sensor_value mag_z;


    while (true) {

    }
}

static void ms5607_processing_callback(int result, uint8_t *buf,uint32_t buf_len, void *userdata) {
    static struct sensor_value pressure;
    static struct sensor_value temperature;

    while (true) {

    }
}

static void tmp117_processing_callback(int result, uint8_t *buf,uint32_t buf_len, void *userdata) {
    static struct sensor_value temperature;

    while (true) {

    }
}



int main(void) {
    printk("Zephyr Example Application %s\n", APP_VERSION_STRING);

    while (1) {
        printk("Launch!\n");
    }

    return 0;
}

