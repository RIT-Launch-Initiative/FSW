#ifndef ADC_READING_H
#define ADC_READING_H

#include <stdint.h>

/**
 * @brief Struct to hold one ADC sample
 */
struct adc_sample {
    /** Timestamp the sample was recorded in ms */
    uint32_t timestamp;
    /** Value of the sample */
    int32_t value;
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
 * @brief Sets ADC control event to start
 */
void adc_start_reading();

/**
 * @brief Sets ADC control event to end
 */
void adc_stop_recording();

#endif // ADC_READING_H