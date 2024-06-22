#ifndef POTATO_H
#define POTATO_H

#include <launch_core_classic/types.h>
#include <stdint.h>

// #ifdef CONFIG_DEBUG
// #define ADC_SAMPLE_COUNT 10
// #define THREAD_START_TIME 0
// #else
#define ADC_SAMPLE_COUNT (128000000 / sizeof(potato_adc_telemetry_t))
#define THREAD_START_TIME 60000 * 5 // 5 minutes
// #endif

typedef enum { PAD_STATE = 0, BOOST_STATE, COAST_STATE, APOGEE_STATE, MAIN_STATE, LANDING_STATE } FLIGHT_STATES;

typedef struct {
    uint8_t parts[3];
} adc_data_t;

#define ASSIGN_V32_TO_ADCDATA(v32, data)                                                                               \
    data.parts[0] = v32 & 0xff;                                                                                        \
    data.parts[1] = (v32 >> 8) & 0xff;                                                                                 \
    data.parts[2] = (v32 >> 16) & 0xff;

typedef struct __attribute__((packed)) {
    l_barometer_data_t lps22_data;
    uint32_t timestamp;
} potato_raw_telemetry_t;

typedef struct __attribute__((packed)) {
    float altitude;
    uint32_t timestamp;
} potato_telemetry_t;

typedef struct __attribute__((packed)) {
    uint32_t timestamp;
    adc_data_t data;
} potato_adc_telemetry_t;

// typedef struct __attribute__((packed)) {
// potato_raw_telemetry_t raw_telemetry;
// potato_telemetry_t telemetry;
// } logging_packet_t;

// Worlds worst orchestration
#define SPIN_WHILE(val, ms)                                                                                            \
    while (val) {                                                                                                      \
        k_msleep(ms);                                                                                                  \
    }

/**
 * Get byte from serial indicating event
 * @return Event byte
 */
uint8_t get_event_from_serial();

#endif //POTATO_H
