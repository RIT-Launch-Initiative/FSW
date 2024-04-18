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

#include "data_storage.h"
#include "testing.h"

// TODO MAKE THIS RIGHT
int32_t timestamp() { return 5; }

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

// Inas
#define INA_BAT_NODE DT_NODELABEL(ina260_battery)
const struct device *ina_bat_dev = DEVICE_DT_GET(INA_BAT_NODE);

#define INA_LDO_NODE DT_NODELABEL(ina260_ldo)
const struct device *ina_ldo_dev = DEVICE_DT_GET(INA_LDO_NODE);

#define INA_GRIM_NODE DT_NODELABEL(ina260_3v3)
const struct device *ina_grim_dev = DEVICE_DT_GET(INA_GRIM_NODE);

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
  gpio_pin_set_dt(&buzzer, 0);

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

  const bool ina_bat_found = device_is_ready(ina_bat_dev);
  const bool ina_ldo_found = device_is_ready(ina_ldo_dev);
  const bool ina_grim_found = device_is_ready(ina_grim_dev);
  const bool all_good2 = ina_bat_found && ina_ldo_found && ina_grim_found;

  if (!all_good2) {
    LOG_ERR("Error setting up INA260 devices");
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

void fatal_buzzer() {
  gpio_pin_set_dt(&ldo_enable, 1);

  while (true) {
    gpio_pin_toggle_dt(&buzzer);
    k_msleep(100);
  }
}

void fast_data_read() {
  int ret = 0;
  ret = sensor_sample_fetch(lsm6dsl_dev);
  if (ret < 0) {
    LOG_WRN("Failed to read IMU: %d", ret);
    return;
  }
  struct sensor_value vals[3];
  sensor_channel_get(lsm6dsl_dev, SENSOR_CHAN_ACCEL_XYZ, vals);
  float ax = sensor_value_to_float(&vals[0]);
  float ay = sensor_value_to_float(&vals[1]);
  float az = sensor_value_to_float(&vals[2]);

  sensor_channel_get(lsm6dsl_dev, SENSOR_CHAN_GYRO_XYZ, vals);
  float gx = sensor_value_to_float(&vals[0]);
  float gy = sensor_value_to_float(&vals[1]);
  float gz = sensor_value_to_float(&vals[2]);

  struct fast_data dat = {
      .timestamp = timestamp(),
      .accel_x = ax,
      .accel_y = ay,
      .accel_z = az,
      .gyro_x = gx,
      .gyro_y = gy,
      .gyro_z = gz,
  };
  k_msgq_put(&fast_data_queue, &dat, K_NO_WAIT);
}

K_SEM_DEFINE(fast_sem, 0, 1);
K_SEM_DEFINE(slow_sem, 0, 1);

void fast_data_alert(struct k_timer *) { k_sem_give(&fast_sem); }
K_TIMER_DEFINE(fast_data_timer, fast_data_alert, NULL);

void slow_data_alert(struct k_timer *) { k_sem_give(&slow_sem); }
K_TIMER_DEFINE(slow_data_timer, slow_data_alert, NULL);

// void fast_data_thread_entry(void *, void *, void *) {
//   while (true) {
// k_sem_take(&fast_sem, K_FOREVER);
// fast_data_read();
// k_sem_give(&fast_sem);
//   }
// }

// K_THREAD_DEFINE(fast_data_reading, 1024, fast_data_thread_entry, NULL, NULL,
// NULL, 25, 0, 0);

int main(void) {
  if (gpio_init()) {
    return -1;
  }
  if (sensor_init()) {
    return -1;
  }
  k_tid_t storage_tid = spawn_data_storage_thread();
  (void)storage_tid;

  // Make sure storage is setup

  if (k_event_wait(&storage_setup_finished, 0xFFF, false, K_FOREVER) ==
      STORAGE_SETUP_FAILED_EVENT) {
    LOG_ERR("Failed to initialize file system. FATAL ERROR\n");
    fatal_buzzer();
    return -1;
  }
  // Storage is ready
  // Start launch detecting
  //   k_timer_start(&fast_data_timer, K_MSEC(1), K_MSEC(1));

  //   for (int i = 0; i < 10000; i++) {
  // fast_data_read();
  // k_msleep(1);
  //   }

  //   k_msleep(10000);

  //   k_timer_stop(&fast_data_timer);

  enum flight_event its_so_over = flight_event_main_shutoff;
  k_msgq_put(&flight_events_queue, &its_so_over, K_NO_WAIT);

  return 0;
}
