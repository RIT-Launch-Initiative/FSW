#pragma once
#include <stdint.h>
#include <zephyr/kernel.h>

struct slow_data {
  uint32_t timestamp;

  uint32_t humidty;
  uint32_t temperature;

  uint16_t grim_voltage;
  uint16_t grim_current;

  uint16_t load_cell_voltage;
  uint16_t load_cell_current;

  uint16_t cam_voltage;
  uint16_t cam_current;
};

struct fast_data {
  uint32_t timestamp;

  uint32_t accel_x;
  uint32_t accel_y;
  uint32_t accel_z;

  uint32_t gyro_x;
  uint32_t gyro_y;
  uint32_t gyro_z;
};

struct adc_data {
  uint32_t timestamp;
  uint32_t adc_value;
};

extern struct k_msgq adc_data_queue;
extern struct k_msgq fast_data_queue;
extern struct k_msgq slow_data_queue;