/*
 * Copyright (c) 2023 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
  if (gpio_pin_configure_dt(&cam_enable, GPIO_OUTPUT_ACTIVE) < 0) {
    LOG_ERR("Unable to configure buzzer output pin\n");
    return -1;
  }

  if (!device_is_ready(debug_serial_dev)) {
    LOG_ERR("CAMERA SERIAL NOT READY\n");
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

  struct sensor_value odr_attr;

  /* set accel/gyro sampling frequency to 104 Hz */
  odr_attr.val1 = 1666;
  odr_attr.val2 = 0;

  if (sensor_attr_set(lsm6dsl_dev, SENSOR_CHAN_ACCEL_XYZ,
                      SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
    printk("Cannot set sampling frequency for accelerometer.\n");
    return 0;
  }

  if (sensor_attr_set(lsm6dsl_dev, SENSOR_CHAN_GYRO_XYZ,
                      SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
    printk("Cannot set sampling frequency for gyro.\n");
    return 0;
  }

  const bool all_good = lsm6dsl_found && flash_found && bme280_found;
  if (!all_good) {
    LOG_ERR("Error setting up sensor and flash devices");
    return -1;
  }

  // Configure channel prior to sampling.
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

int main(void) {
  if (gpio_init()) {
    return -1;
  }
  if (sensor_init()) {
    return -1;
  }

  int32_t buf;
  struct adc_sequence sequence = {
      .buffer = &buf,
      /* buffer size in bytes, not number of samples */
      .buffer_size = sizeof(buf),
  };

  while (true) {
    // convert_and_send();
    gpio_pin_toggle_dt(&led1);
    gpio_pin_toggle_dt(&led2);
    // gpio_pin_toggle_dt(&ldo_enable);
    // gpio_pin_toggle_dt(&cam_enable);
    // gpio_pin_toggle_dt(&buzzer);

    int err = adc_sequence_init_dt(&adc_chan0, &sequence);
    if (err < 0) {
      printk("Could not init adc seq: %d", err);
    }
    err = adc_read_dt(&adc_chan0, &sequence);
    if (err < 0) {
      printk("Could not read adc channel %d (%d)\n", adc_chan0.channel_id, err);
      continue;
    }
    // int32_t value_mv = (buf * 2400) / (1 << 23);
    // err = adc_raw_to_millivolts_dt(&adc_chan0, &value_mv);
    if (err < 0) {
      LOG_ERR("Could not convert value to mv: Err: %d", err);
    }
    printk("Value: %06x, %d mv, res: %d\n", buf, 0, adc_chan0.resolution);
    k_sleep(K_MSEC(2500));
  }
  return 0;
}
