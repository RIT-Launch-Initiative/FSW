#ifndef POTATO_H
#define POTATO_H

#include <stdint.h>

#include <launch_core/types.h>

typedef struct {
    uint32_t timestamp;
    l_barometer_data_t lps22_data;
    float load;
} potato_raw_telemetry_t;

typedef struct {
    uint32_t timestamp;
    float altitude;
    float load;
} potato_telemetry_t;

#endif //POTATO_H
