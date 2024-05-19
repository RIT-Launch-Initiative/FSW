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

#endif //POTATO_H
