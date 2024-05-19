#ifndef POTATO_H
#define POTATO_H

#include <stdint.h>

#include <launch_core/types.h>

typedef struct __attribute__((packed)) {
    l_barometer_data_t lps22_data;
    float load;
    uint32_t timestamp;
} potato_raw_telemetry_t;

typedef struct __attribute__((packed)) {
    float altitude;
    float load;
    uint32_t timestamp;
} potato_telemetry_t;

typedef struct __attribute__((packed)) {
    potato_raw_telemetry_t raw_telemetry;
    potato_telemetry_t telemetry;
} logging_packet_t;

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
