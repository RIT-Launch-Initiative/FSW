/*
 * Copyright (c) 2023 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "config.h"

#include <math.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>

#include "data_storage.h"
#include "testing.h"
#include <launch_core/dev/dev_common.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/shell/shell.h>
#include <zephyr/sys/base64.h>

// Nasty, put these in the right place when you have time
#include "ina260.h"
#include <zephyr/../../drivers/sensor/lsm6dsl/lsm6dsl.h>
// TODO MAKE THIS RIGHT
int32_t timestamp() {
  int32_t ms = k_uptime_get();
  return ms * 1000;
}

LOG_MODULE_REGISTER(main);

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

// static inline ALWAYS_INLINE void get_likely_or_warn(const int res,
//                                                     const char *msg) {
#define get_likely_or_warn(res, msg)                                           \
  if (unlikely(res < 0)) {                                                     \
    LOG_WRN("%s: %d", msg, res);                                               \
  }
// }
volatile int num_samples_fast = 0;
volatile int timestamp_over_limit = -1;
volatile int over_limit_for = 0;
void fast_data_read(struct k_work *) {

  // Read Data
  int ret = sensor_sample_fetch_chan(lsm6dsl_dev, SENSOR_CHAN_ACCEL_XYZ);
  get_likely_or_warn(ret, "Failed to read IMU accel");
  ret = sensor_sample_fetch_chan(lsm6dsl_dev, SENSOR_CHAN_GYRO_XYZ);
  get_likely_or_warn(ret, "Failed to read IMU gyro");
  // Store data

  struct lsm6dsl_data *data = lsm6dsl_dev->data;
  struct fast_data dat = {
      .timestamp = timestamp(),
      .accel_x = data->accel_sample_x,
      .accel_y = data->accel_sample_y,
      .accel_z = data->accel_sample_z,
      .gyro_x = data->gyro_sample_x,
      .gyro_y = data->gyro_sample_y,
      .gyro_z = data->gyro_sample_z,
  };

  ret = k_msgq_put(&fast_data_queue, &dat, K_NO_WAIT);
  get_likely_or_warn(ret, "Failed to send fast message");

  // Launch Detect
  struct sensor_value accel_vals[3];
  get_likely_or_warn(
      sensor_channel_get(lsm6dsl_dev, SENSOR_CHAN_ACCEL_XYZ, accel_vals),
      "Failed to convert accel values");
  int acc_mag = (accel_vals[0].val1 * accel_vals[0].val1) +
                (accel_vals[1].val1 * accel_vals[1].val1) +
                (accel_vals[2].val1 * accel_vals[2].val1);
  if (acc_mag > FIVE_G_LIMIT_MPS * FIVE_G_LIMIT_MPS) {
    over_limit_for++;
    printk("Over limit: sqrt %d\n", acc_mag);
  }

  float ax = sensor_value_to_float(&accel_vals[0]);

  //   float ay = sensor_value_to_float(&accel_vals[1]);
  //   float az = sensor_value_to_float(&accel_vals[2]);
  //   float acc_mag = sqrtf((ax * ax) + (ay * ay) + (az * az));
  // Only need to read pressure at 100hz
  //   if (num_samples_fast % 10 == 0) {
  // struct sensor_value press;
  // get_likely_or_warn(sensor_sample_fetch_chan(bme280_dev, SENSOR_CHAN_PRESS),
  //    "unable to fetch pressure");
  // float pressure = sensor_value_to_float(&press);
  //   }
  //
  num_samples_fast++;
}

volatile int num_samples_slow = 0;
void slow_data_read(struct k_work *) {
  struct sensor_value temp;
  struct sensor_value press;
  struct sensor_value humid;
  // BME is read by fast sensor thread for event detection. No need to fetch it
  // here just read it
  get_likely_or_warn(sensor_sample_fetch(bme280_dev),
                     "Unable to fetch bme280 readings");

  get_likely_or_warn(
      sensor_channel_get(bme280_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp),
      "Unable to read ambient temp");
  get_likely_or_warn(sensor_channel_get(bme280_dev, SENSOR_CHAN_PRESS, &press),
                     "Unable to read pressure");

  get_likely_or_warn(
      sensor_channel_get(bme280_dev, SENSOR_CHAN_HUMIDITY, &humid),
      "Unable to read humidity");

  // Voltages
  get_likely_or_warn(sensor_sample_fetch(ina_bat_dev),
                     "Unable to fetch battery INA");
  get_likely_or_warn(sensor_sample_fetch(ina_grim_dev),
                     "Unable to fetch battery INA");
  get_likely_or_warn(sensor_sample_fetch(ina_ldo_dev),
                     "Unable to fetch LDO INA");

  struct ina260_data *bat_data = ina_bat_dev->data;
  struct ina260_data *ldo_data = ina_ldo_dev->data;
  struct ina260_data *grim_data = ina_grim_dev->data;

  struct slow_data dat = {
      .timestamp = timestamp(),
      .humidity = humid.val1 * 1000000 + humid.val2,
      .temperature = temp.val1 * 1000000 + temp.val2,
      .grim_voltage = grim_data->v_bus,
      .grim_current = grim_data->current,

      .load_cell_voltage = ldo_data->v_bus,
      .load_cell_current = ldo_data->current,

      .bat_voltage = bat_data->v_bus,
      .bat_current = bat_data->current,
  };
  get_likely_or_warn(k_msgq_put(&slow_data_queue, &dat, K_NO_WAIT),
                     "Failed to send slow message");
  num_samples_slow++;
}

K_WORK_DEFINE(fast_work, fast_data_read);
void fast_data_alert(struct k_timer *) { k_work_submit(&fast_work); }
K_TIMER_DEFINE(fast_data_timer, fast_data_alert, NULL);

K_WORK_DEFINE(slow_work, slow_data_read);
void slow_data_alert(struct k_timer *) { k_work_submit(&slow_work); }
K_TIMER_DEFINE(slow_data_timer, slow_data_alert, NULL);

int main(void) {
  if (gpio_init()) {
    fatal_buzzer();
    return -1;
  }
  if (sensor_init()) {
    fatal_buzzer();
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
  LOG_DBG("Starting launch detection");
  // Storage is ready
  // Start launch detecting
  gpio_pin_set_dt(&cam_enable, 1);
  gpio_pin_set_dt(&ldo_enable, 1);

  k_timer_start(&fast_data_timer, FAST_DATA_DELAY_MS, FAST_DATA_DELAY_MS);
  k_timer_start(&slow_data_timer, SLOW_DATA_DELAY_MS, SLOW_DATA_DELAY_MS);

  k_msleep(FLIGHT_TIME_MS);

  k_timer_stop(&fast_data_timer);
  k_timer_stop(&slow_data_timer);

  enum flight_event its_so_over = flight_event_main_shutoff;
  k_msgq_put(&flight_events_queue, &its_so_over, K_NO_WAIT);

  LOG_DBG("Samples: %d", num_samples_fast);
  printk("Fast: %d, Slow: %d\n", num_samples_fast, num_samples_slow);
  printk("Over limit for: %d\n", over_limit_for);

  return 0;
}

// Dumping

// read from file into this
#define B64_PER_LINE (12 * 6)
#define READING_BUFFER_LEN B64_PER_LINE
uint8_t reading_data[READING_BUFFER_LEN];

// dump base64 data here before printing
#define DUMP_BUFFER_LEN (B64_PER_LINE * 8)
uint8_t dumping_data[DUMP_BUFFER_LEN + 1];

static int dump_base64(const struct shell *shell, const char *fname) {
  struct fs_file_t file;
  int ret;
  fs_file_t_init(&file);

  ret = fs_open(&file, fname, FS_O_READ);
  if (ret < 0) {
    shell_print(shell, "Failed to open %s: %d", fname, ret);
    return ret;
  }
  int read = READING_BUFFER_LEN;

  while (read == READING_BUFFER_LEN) {
    read = fs_read(&file, reading_data, READING_BUFFER_LEN);
    if (read < 0) {
      shell_print(shell, "Failed file read of %s", fname);
      return -1;
    }
    int num_written = 0;
    ret = base64_encode(dumping_data, DUMP_BUFFER_LEN, &num_written,
                        reading_data, read);
    if (ret < 0) {
      shell_print(shell, "encoding error %d, num_written: %d, read: %d", ret,
                  num_written, read);
      return ret;
    }
    dumping_data[DUMP_BUFFER_LEN] = 0; // null terminator
    shell_print(shell, "%s", dumping_data);
  }
  fs_close(&file);
  return 0;
}

static int cmd_dump_fast(const struct shell *shell, size_t argc, char **argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  dump_base64(shell, FAST_FILENAME);
  return 0;
}

static int cmd_dump_slow(const struct shell *shell, size_t argc, char **argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  dump_base64(shell, SLOW_FILENAME);
  return 0;
}

static int cmd_dump_adc(const struct shell *shell, size_t argc, char **argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);
  dump_base64(shell, ADC_FILENAME);
  return 0;
}

static int cmd_nogo(const struct shell *shell, size_t argc, char **argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  LOG_ERR("NOGO Unimplemented");

  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    dump_subcmds, SHELL_CMD(nogo, NULL, "Cancel launch detection", cmd_nogo),
    SHELL_CMD(dump_slow, NULL, "Dump slow file.", cmd_dump_slow),
    SHELL_CMD(dump_fast, NULL, "Dump fast file.", cmd_dump_fast),
    SHELL_CMD(dump_adc, NULL, "Dump adc file.", cmd_dump_adc),
    SHELL_SUBCMD_SET_END);

/* Creating root (level 0) command "demo" */
SHELL_CMD_REGISTER(dump, &dump_subcmds, "Data Dump Commands", NULL);
