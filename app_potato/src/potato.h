#ifndef POTATO_H
#define POTATO_H

#include <launch_core/types.h>
#include <stdint.h>

#define ADC_READINGS_PER_PACKET 10
#define ADC_PERIOD              K_USEC(101)

typedef enum { PAD_STATE = 0, BOOST_STATE, COAST_STATE, APOGEE_STATE, MAIN_STATE, LANDING_STATE } FLIGHT_STATES;

typedef struct __attribute__((packed)) {
    uint8_t data[3];
} adc_data_t;

#define I32_TO_ADCDATA(v32)                                                                                            \
    { v32 & 0xff, (v32 >> 8) & 0xff, (v32 >> 16) & 0xff }

typedef struct __attribute__((packed)) {
    l_barometer_data_t lps22_data;
    uint32_t timestamp;
} potato_raw_telemetry_t;

typedef struct __attribute__((packed)) {
    float altitude;
    uint32_t timestamp;
} potato_telemetry_t;

typedef struct __attribute__((packed)) {
    adc_data_t data[ADC_READINGS_PER_PACKET];
    uint32_t timestamp;
} potato_adc_telemetry_t;

typedef struct __attribute__((packed)) {
    potato_raw_telemetry_t raw_telemetry;
    potato_telemetry_t telemetry;
} logging_packet_t;

/**
 * Start boost detection checking
 */
void start_boost_detect();

/**
 * Stop boost detection checking
 */
void stop_boost_detect();

/**
 * Get byte from serial indicating event
 * @return Event byte
 */
uint8_t get_event_from_serial();

/**
 * Configure telemetry rate for POTATO
 * @param frequency - Frequency of sensor sampling
 */
void configure_telemetry_rate(uint32_t frequency);

/**
 * Bin a telemetry file for flight phase transitions
 */
void bin_telemetry_file();

#endif //POTATO_H
