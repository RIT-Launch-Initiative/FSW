/*
 * Copyright (c) 2023 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "buzzer.h"
#include "config.h"
#include "data_storage.h"
#include "ina260.h"

#include <launch_core/dev/dev_common.h>
#include <math.h>
#include <zephyr/../../drivers/sensor/lsm6dsl/lsm6dsl.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/storage/flash_map.h>
// TODO MAKE THIS RIGHT
int32_t timestamp() {
    int32_t us = k_ticks_to_us_floor32(k_uptime_ticks());
    return us;
}
#define MAX_LOG_LEVEL 4
LOG_MODULE_REGISTER(main, MAX_LOG_LEVEL);

// devicetree gets
#define LED1_NODE DT_NODELABEL(led1)
#define LED2_NODE DT_NODELABEL(led2)

const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

#define LDO_EN_NODE DT_NODELABEL(ldo_enable)
#define CAM_EN_NODE DT_NODELABEL(cam_enable)

const struct gpio_dt_spec ldo_enable = GPIO_DT_SPEC_GET(LDO_EN_NODE, gpios);
const struct gpio_dt_spec cam_enable = GPIO_DT_SPEC_GET(CAM_EN_NODE, gpios);

#define BUZZER_NODE DT_NODELABEL(buzzer)
const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(BUZZER_NODE, gpios);

#define DBG_SERIAL_NODE DT_ALIAS(debug_serial)
const struct device *const debug_serial_dev = DEVICE_DT_GET(DBG_SERIAL_NODE);

#define BME280_NODE DT_NODELABEL(bme280)
const struct device *bme280_dev = DEVICE_DT_GET(BME280_NODE);

#define LSM6DSL_NODE DT_NODELABEL(lsm6dsl)
const struct device *lsm6dsl_dev = DEVICE_DT_GET(LSM6DSL_NODE);

#define FLASH_NODE DT_NODELABEL(w25q512)
const struct device *flash_dev = DEVICE_DT_GET(FLASH_NODE);

// Inas
#define INA_BAT_NODE DT_NODELABEL(ina260_battery)
const struct device *ina_bat_dev = DEVICE_DT_GET(INA_BAT_NODE);

#define INA_LDO_NODE DT_NODELABEL(ina260_ldo)
const struct device *ina_ldo_dev = DEVICE_DT_GET(INA_LDO_NODE);

#define INA_GRIM_NODE DT_NODELABEL(ina260_3v3)
const struct device *ina_grim_dev = DEVICE_DT_GET(INA_GRIM_NODE);

static const struct adc_dt_spec adc_chan0 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

#define INIT_GPIO_FAIL      -1
#define INIT_NOFLASH        -1
#define INIT_MISSING_SENSOR -2
#define INIT_OK             0

static int gpio_init(void) {
    // Init LEDS
    if (!gpio_is_ready_dt(&led1)) {
        LOG_ERR("LED 1 is not ready\n");
        return INIT_GPIO_FAIL;
    }
    if (gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE) < 0) {
        LOG_ERR("Unable to configure LED 1 output pin\n");
        return INIT_GPIO_FAIL;
    }

    if (!gpio_is_ready_dt(&led2)) {
        LOG_ERR("LED 2 is not ready\n");
        return INIT_GPIO_FAIL;
    }
    if (gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE) < 0) {
        LOG_ERR("Unable to configure LED 2 output pin\n");
        return INIT_GPIO_FAIL;
    }
    // Init Enable pins
    if (!gpio_is_ready_dt(&ldo_enable)) {
        LOG_ERR("ldo enable pin is not ready\n");
        return INIT_GPIO_FAIL;
    }
    if (gpio_pin_configure_dt(&ldo_enable, GPIO_OUTPUT_ACTIVE) < 0) {
        LOG_ERR("Unable to configure ldo enable output pin\n");
        return INIT_GPIO_FAIL;
    }

    if (!gpio_is_ready_dt(&cam_enable)) {
        LOG_ERR("camera enable pin is not ready\n");
        return INIT_GPIO_FAIL;
    }
    if (gpio_pin_configure_dt(&cam_enable, GPIO_OUTPUT_ACTIVE) < 0) {
        LOG_ERR("Unable to configure camera enable output pin\n");
        return INIT_GPIO_FAIL;
    }

    if (!gpio_is_ready_dt(&buzzer)) {
        LOG_ERR("buzzer pin is not ready\n");
        return INIT_GPIO_FAIL;
    }
    if (gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT_ACTIVE) < 0) {
        LOG_ERR("Unable to configure buzzer output pin\n");
        return INIT_GPIO_FAIL;
    }

    if (!device_is_ready(debug_serial_dev)) {
        LOG_ERR("Debug serial not ready\n");
        return INIT_GPIO_FAIL;
    }

    gpio_pin_set_dt(&led1, 0);
    gpio_pin_set_dt(&led2, 0);
    gpio_pin_set_dt(&ldo_enable, 1);
    gpio_pin_set_dt(&cam_enable, 0);
    gpio_pin_set_dt(&buzzer, 0);

    return INIT_OK;
}

static int sensor_init(void) {
    const bool flash_found = device_is_ready(flash_dev);
    if (!flash_found) {
        return INIT_NOFLASH;
    }

    const bool lsm6dsl_found = device_is_ready(lsm6dsl_dev);
    const bool bme280_found = device_is_ready(bme280_dev);
    if (!lsm6dsl_found) {
        LOG_ERR("Error setting up LSM6DSL");
        return INIT_MISSING_SENSOR;
    }
    if (!bme280_found) {
        LOG_ERR("Error setting up BME280");
        return INIT_MISSING_SENSOR;
    }
    const bool ina_bat_found = device_is_ready(ina_bat_dev);
    const bool ina_ldo_found = device_is_ready(ina_ldo_dev);
    const bool ina_grim_found = device_is_ready(ina_grim_dev);

    if (!ina_bat_found || !ina_ldo_found || !ina_grim_found) {
        LOG_ERR("Error setting up INA260 devices");
        return INIT_MISSING_SENSOR;
    }

    // ADC
    if (!adc_is_ready_dt(&adc_chan0)) {
        LOG_ERR("ADC controller device %s not ready\n", adc_chan0.dev->name);
        return INIT_MISSING_SENSOR;
    }
    //
    if (adc_channel_setup_dt(&adc_chan0) < 0) {
        LOG_ERR("Could not setup ADC channel\n");
        return INIT_MISSING_SENSOR;
    }

    return 0;
}

int main(void) {

    if (gpio_init() != 0) {
        LOG_ERR("GPIO not setup. Continuing...\n");
        buzzer_tell(buzzer_cond_missing_sensors);
    }
    int ret = sensor_init();
    if (ret == INIT_NOFLASH) {
        LOG_ERR("Flash is not functional");
        buzzer_tell(buzzer_cond_noflash);
    } else if (ret == INIT_MISSING_SENSOR) {
        LOG_ERR("Some sensors not functional");
        buzzer_tell(buzzer_cond_missing_sensors);
    }

    begin_buzzer_thread(&buzzer);

    k_tid_t storage_tid = spawn_data_storage_thread();
    (void) storage_tid;

    // Make sure storage is setup
    if (k_event_wait(&storage_setup_finished, 0xFFFFFFFF, false, K_FOREVER) == STORAGE_SETUP_FAILED_EVENT) {
        LOG_ERR("Failed to initialize file sysbegin_buzzer_threadtem. FATAL ERROR\n");
        buzzer_tell(buzzer_cond_noflash);
        // VERY VERY BAD
    }

    LOG_DBG("Starting launch detection");

    LOG_INF("Landed!");
    buzzer_tell(buzzer_cond_landed);
    return 0;
}
