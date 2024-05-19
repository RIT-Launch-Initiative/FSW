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

/**
 * Check if a gpio is ready or return a status code indicating that its not.
 * gpio: gpio_dt_spec
 * name: a string describing this pin. gpio_dt_spec only knows the name of its gpio controller device so make this a friendlier name
*/
#define CHECK_GPIO_OUTPUT(gpio, name)                                                                                  \
    if (!gpio_is_ready_dt(&gpio)) {                                                                                    \
        LOG_ERR("%s is not ready", name);                                                                              \
        return -ENODEV;                                                                                                \
    }                                                                                                                  \
    if (gpio_pin_configure_dt(&gpio, GPIO_OUTPUT_ACTIVE) < 0) {                                                        \
        LOG_ERR("Unable to configure %s output pin\n", name);                                                          \
        return -ENODEV;                                                                                                \
    }

/**
 * Check if a given device is ready. Returning -ENODEV if it's not
*/
#define CHECK_DEVICE_READY(dev)                                                                                        \
    if (!device_is_ready(dev)) {                                                                                       \
        LOG_ERR("Device %s is not ready.", dev->name);                                                                 \
        return -ENODEV;                                                                                                \
    }

/**
 * @brief Setup all the gpio pins needed for flight
 * @return -ENODEV if a GPIO couldnt be setup. 0 otherwise
 */
static int gpio_init(void) {
    // Init LEDS
    CHECK_GPIO_OUTPUT(led1, "led1");
    CHECK_GPIO_OUTPUT(led2, "led2");
    CHECK_GPIO_OUTPUT(ldo_enable, "ldo_enable");
    CHECK_GPIO_OUTPUT(cam_enable, "cam_enable");
    CHECK_GPIO_OUTPUT(buzzer, "buzzer");

    gpio_pin_set_dt(&led1, 0);
    gpio_pin_set_dt(&led2, 0);
    gpio_pin_set_dt(&ldo_enable, 1);
    gpio_pin_set_dt(&cam_enable, 0);
    gpio_pin_set_dt(&buzzer, 0);

    return 0;
}
/**
 * @brief Initialize flash and all sensors. Flash, bme280, adc, and inas
 * @return -EIO if flash is not found. -ENODEV if any sensor isn't ready. 0 if all ok 
 */
static int sensor_init(void) {
    const bool flash_found = device_is_ready(flash_dev);
    if (!flash_found) {
        return -EIO;
    }
    CHECK_DEVICE_READY(lsm6dsl_dev)
    CHECK_DEVICE_READY(bme280_dev)

    CHECK_DEVICE_READY(ina_bat_dev)
    CHECK_DEVICE_READY(ina_ldo_dev)
    CHECK_DEVICE_READY(ina_grim_dev)

    // ADC
    if (!adc_is_ready_dt(&adc_chan0)) {
        LOG_ERR("ADC controller device %s not ready\n", adc_chan0.dev->name);
        return -ENODEV;
    }
    int ret = adc_channel_setup_dt(&adc_chan0);
    if (ret < 0) {
        LOG_ERR("Could not setup ADC channel: %d\n", ret);
        return -ENODEV;
    }

    CHECK_DEVICE_READY(debug_serial_dev)

    return 0;
}

int main(void) {

    if (gpio_init() != 0) {
        LOG_ERR("GPIO not setup. Continuing...\n");
        buzzer_tell(buzzer_cond_missing_sensors);
    }
    int ret = sensor_init();
    if (ret == -EIO) {
        LOG_ERR("Flash is not functional");
        buzzer_tell(buzzer_cond_noflash);
    } else if (ret == -ENODEV) {
        LOG_ERR("Some sensors not functional");
        buzzer_tell(buzzer_cond_missing_sensors);
    }

    begin_buzzer_thread(&buzzer);

    (void) spawn_data_storage_thread();

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
