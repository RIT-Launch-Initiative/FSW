/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <app_version.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL
);


static int print_samples;
static int lsm6dsl_trig_cnt;

static void adxl375_processing_callback(int result, uint8_t *buf, uint32_t buf_len, void *userdata) {
    static struct sensor_value accel_x;
    static struct sensor_value accel_y;
    static struct sensor_value accel_z;

    while (true) {

    }
}

static void bmp388_processing_callback(int result, uint8_t *buf, uint32_t buf_len, void *userdata) {
    static struct sensor_value pressure;
    static struct sensor_value temperature;

    while (true) {

    }
}

static void lsm6dsl_processing_callback(int result, uint8_t *buf, uint32_t buf_len, void *userdata) {
    static struct sensor_value accel_x;
    static struct sensor_value accel_y;
    static struct sensor_value accel_z;
    static struct sensor_value gyro_x;
    static struct sensor_value gyro_y;
    static struct sensor_value gyro_z;

    while (true) {

    }
}

static void lis3mdl_processing_callback(int result, uint8_t *buf, uint32_t buf_len, void *userdata) {
    static struct sensor_value mag_x;
    static struct sensor_value mag_y;
    static struct sensor_value mag_z;


    while (true) {

    }
}

static void ms5607_processing_callback(int result, uint8_t *buf, uint32_t buf_len, void *userdata) {
    static struct sensor_value pressure;
    static struct sensor_value temperature;

    while (true) {

    }
}

static void tmp117_processing_callback(int result, uint8_t *buf, uint32_t buf_len, void *userdata) {
    static struct sensor_value temperature;

    while (true) {

    }
}

static void ina219_processing() {
    while (true) {

    }
}

//int main(void) {
//    const struct device *const rfm = DEVICE_DT_GET_ONE(rfm95w);
//    struct lora_modem_config = {
//            .frequency = 91500000,
//            .bandwith = BW_125_KHZ,
//            .datarate = SF_10,
//            .preamble_len = 8,
//            .coding_rate = CR_4_5,
//            .iq_inverted = false,
//            .public_network = false,
//            .tx_power = 4,
//            .tx = true
//    };
//
//    int ret = lora_config(lora_modem_config);
//    if (ret != 0) {
//        printf("LORA Configuration failed!");
//        return 0;
//    }
//
//    while (1) {
//        ret = lora_send(lora_dev, data, 256);
//        if (ret != 0) {
//            printf("LORA Send Failed!");
//        }
//
//        k_sleep(1000);
//    }
//}

//int main(void) {
//    const struct device *const ina = DEVICE_DT_GET_ONE(ti_ina219);
//    struct sensor_value v_bus;
//    struct sensor_value power;
//    struct sensor_value current;
//
//    if (!device_is_ready(ina)) {
//        printf("Device %s is not ready.\n", ina->name);
//        return 0;
//    }
//
//    while (true) {
//        if (sensor_sample_fetch(ina)) {
//            printf("Could not fetch sensor data.\n");
//            return 0;
//        }
//
//        sensor_channel_get(ina, SENSOR_CHAN_VOLTAGE, &v_bus);
//        sensor_channel_get(ina, SENSOR_CHAN_POWER, &power);
//        sensor_channel_get(ina, SENSOR_CHAN_CURRENT, &current);
//
//        printf("Bus: %f [V] -- "
//               "Power: %f [W] -- "
//               "Current: %f [A]\n",
//               sensor_value_to_double(&v_bus),
//               sensor_value_to_double(&power),
//               sensor_value_to_double(&current));
//        k_sleep(K_MSEC(2000));
//    }
//
//    return 0;
//}


int main(void) {
    printk("Zephyr Example Application %s\n", APP_VERSION_STRING);

    while (1) {
        printk("Launch!\n");
    }

    return 0;
}

