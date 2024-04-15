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

  //   if (!gpio_is_ready_dt(&buzzer)) {
  // LOG_ERR("buzzer pin is not ready\n");
  // return -1;
  //   }
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

#define NUM_SAMPLES 5
float samples[NUM_SAMPLES] = {0.0f};
int sample_index = 0;
float sample_sum = 0;

int samplesi[NUM_SAMPLES] = {0.0f};
int samplei_index = 0;
float samplei_sum = 0;

// add a new sample, return the moving avg
float add_sample(float sample) {
  float old = samples[sample_index];
  samples[sample_index] = sample;
  sample_index = (sample_index + 1) % NUM_SAMPLES;
  sample_sum -= old;
  sample_sum += sample;
  return sample_sum / (float)NUM_SAMPLES;
}

float add_sample_int(int samplei) {
  float old = samplesi[samplei_index];
  samplesi[samplei_index] = samplei;
  samplei_index = (samplei_index + 1) % NUM_SAMPLES;
  samplei_sum -= old;
  samplei_sum += samplei;
  return samplei_sum / (float)NUM_SAMPLES;
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
  int frame = 0;
  while (true) {

    sequence.channels = adc_chan0.channel_id;

    int err = adc_sequence_init_dt(&adc_chan0, &sequence);
    if (err < 0) {
      printk("Could not init seq: %d", err);
    }
    err = adc_read_dt(&adc_chan0, &sequence);
    if (err < 0) {
      printk("Could not read (%d)\n", err);
      continue;
    }
    int32_t val = buf;
    float volts = 2.4f * ((float)val) / ((float)0x7fffff);

    float avg = add_sample(volts);
    int avgi = add_sample_int(val);

    printk("%06d,%06x,%d,%2.8f,%2.8f\n", frame, val, val, (double)volts,
           (double)avg);
    frame++;

    // printk("Im alive\n");
    gpio_pin_toggle_dt(&led1);
    k_msleep(10);
  }
  return 0;
}
