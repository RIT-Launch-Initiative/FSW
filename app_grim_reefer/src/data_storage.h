#ifndef REEFER_INLCUDE_DATA_STORAGE_H
#define REEFER_INLCUDE_DATA_STORAGE_H

#include <launch_core/types.h>
#include <stdint.h>
#include <zephyr/kernel.h>
struct slow_data {
    uint32_t timestamp;

    float humidity;
    float temperature;

    uint16_t grim_voltage; // LSB =  1.25 mV
    uint16_t grim_current; // LSB =  1.25 mA

    uint16_t load_cell_voltage; // LSB =  1.25 mV
    uint16_t load_cell_current; // LSB =  1.25 mA

    uint16_t bat_voltage; // LSB =  1.25 mV
    uint16_t bat_current; // LSB =  1.25 mA
};

struct fast_data {
    uint32_t timestamp;

    l_accelerometer_data_t acc;
    l_gyroscope_data_t gyro;
    float pressure;
};

struct adc_data {
    uint32_t timestamp;
    uint32_t adc_value[10];
};

/**
 * @brief Open files, setup thread and get ready for data to start streaming in
 * @return 0 if setup correctly. -1 if not
 */
int start_data_storage_thread();
/**
 * @brief Stop accepting new data and save everything to disk
 */
void finish_data_storage();

#endif