#ifndef ADC_READING_H
#define ADC_READING_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Struct to hold one ADC sample
 */
struct adc_sample {
    uint32_t timestamp; /** Timestamp the sample was recorded in ms */
    int32_t value;      /** Value of the sample */
};

/**
 * @brief Initialize the ADC device and channel
 * @return 0 if successful
 */
int adc_init();

/**
 * @brief Read one ADC sample
 * @param adc_val Pointer to value where sample will be written
 */
void adc_read_one(uint32_t *adc_val);

/**
 * @brief Waits for ADC reading event to start, then reads ADC samples for 10 seconds
 */
void adc_reading_task();

/**
 * @brief Starts test
 * @param terminal_test Whether test was triggered by terminal cmd or meep
 *                      If test was triggered by terminal, ematch will NOT light
 */
void adc_start_reading(bool terminal_test);

/**
 * @brief Stops test
 */
void adc_stop_recording();

#endif // ADC_READING_H