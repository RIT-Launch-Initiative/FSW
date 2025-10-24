#ifndef ADC_READING_H
#define ADC_READING_H

#include <stdint.h>

struct adc_sample {
    uint32_t timestamp;
    int32_t value;
};

int adc_init();
void adc_reading_task();
void adc_start_reading();
void adc_stop_recording();

#endif // ADC_READING_H