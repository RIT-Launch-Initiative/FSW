/*
 * Copyright (c) 2023 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "config.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>

#include <launch_core/dev/dev_common.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

#include "testing.h"
#include <zephyr/fs/fs.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_GRIM_REEFER_LOG_LEVEL_DBG);

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

static const struct adc_dt_spec adc_chan0 =
    ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

static int gpio_init(void) {
  // Init LEDS
  if (!gpio_is_ready_dt(&led1)) {
    LOG_ERR("LED 1 is not ready\n");
    return -1;
  }
  if (gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE) < 0) {
    LOG_ERR("Unable to configure LED 1 output pin\n");
    return -1;
  }

  if (!gpio_is_ready_dt(&led2)) {
    LOG_ERR("LED 2 is not ready\n");
    return -1;
  }
  if (gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE) < 0) {
    LOG_ERR("Unable to configure LED 2 output pin\n");
    return -1;
  }
  // Init Enable pins
  if (!gpio_is_ready_dt(&ldo_enable)) {
    LOG_ERR("ldo enable pin is not ready\n");
    return -1;
  }
  if (gpio_pin_configure_dt(&ldo_enable, GPIO_OUTPUT_ACTIVE) < 0) {
    LOG_ERR("Unable to configure ldo enable output pin\n");
    return -1;
  }

  if (!gpio_is_ready_dt(&cam_enable)) {
    LOG_ERR("camera enable pin is not ready\n");
    return -1;
  }
  if (gpio_pin_configure_dt(&cam_enable, GPIO_OUTPUT_ACTIVE) < 0) {
    LOG_ERR("Unable to configure camera enable output pin\n");
    return -1;
  }

  if (!gpio_is_ready_dt(&buzzer)) {
    LOG_ERR("buzzer pin is not ready\n");
    return -1;
  }
  if (gpio_pin_configure_dt(&buzzer, GPIO_OUTPUT_ACTIVE) < 0) {
    LOG_ERR("Unable to configure buzzer output pin\n");
    return -1;
  }

  if (!device_is_ready(debug_serial_dev)) {
    LOG_ERR("Debug serial not ready\n");
    return -1;
  }

  gpio_pin_set_dt(&led1, 0);
  gpio_pin_set_dt(&led2, 0);
  gpio_pin_set_dt(&ldo_enable, 0);
  gpio_pin_set_dt(&cam_enable, 0);

  return 0;
}

static int sensor_init(void) {
  const bool lsm6dsl_found = device_is_ready(lsm6dsl_dev);
  const bool bme280_found = device_is_ready(bme280_dev);
  const bool flash_found = device_is_ready(flash_dev);
  const bool all_good = lsm6dsl_found && flash_found && bme280_found;
  if (!all_good) {
    LOG_ERR("Error setting up sensor and flash devices");
    return -1;
  }

  // ADC
  if (!adc_is_ready_dt(&adc_chan0)) {
    LOG_ERR("ADC controller device %s not ready\n", adc_chan0.dev->name);
    return -1;
  }
  //
  if (adc_channel_setup_dt(&adc_chan0) < 0) {
    LOG_ERR("Could not setup channel\n");
    return -1;
  }

  return 0;
}
void print_size(size_t size) {
  if (size < 1024) {
    LOG_PRINTK("%ud B", size);
  } else if (size < 1024 * 1024) {
    LOG_PRINTK("%.2f KiB", ((double)size) / 1024);
  } else {
    LOG_PRINTK("%.2f MiB", ((double)size) / (1024 * 1024));
  }
}
/**
 * @brief Print the stats of the file system the file is on
 * @param fname		file name
 */
void print_statvfs(char *fname) {
  struct fs_statvfs fs_stat_dst; // destination for file system stats
  int32_t ret = fs_statvfs(fname, &fs_stat_dst);
  if (ret < 0) {
    LOG_ERR("Unable to stat the filesystem of %s: %d", fname, ret);
  } else {
    LOG_INF(
        "%s is on a volume with \r\n\t%lu blocks (%lu free) of %lu bytes each",
        fname, fs_stat_dst.f_blocks, fs_stat_dst.f_bfree, fs_stat_dst.f_frsize);
    LOG_PRINTK("\t");
    print_size(fs_stat_dst.f_blocks * fs_stat_dst.f_frsize);
    LOG_PRINTK(" (");
    print_size(fs_stat_dst.f_bfree * fs_stat_dst.f_frsize);
    LOG_PRINTK(" free)\n");
  }
}

int main(void) {
  if (gpio_init()) {
    return -1;
  }
  if (sensor_init()) {
    return -1;
  }
  while (true) {
    gpio_pin_toggle_dt(&led1);
    k_msleep(100);
  }
  return 0;
}
