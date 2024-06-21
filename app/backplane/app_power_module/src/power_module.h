#ifndef POWER_MODULE_H
#define POWER_MODULE_H

#include <launch_core/backplane_defs.h>
#include <launch_core/net/net_common.h>
#include <launch_core/types.h>

#define POWER_MODULE_IP_ADDR BACKPLANE_IP(POWER_MODULE_ID, 2, 1) // TODO: Make this configurable

typedef enum {
    GROUND_STATE = 0,
    FLIGHT_STATE,
} FLIGHT_STATES;

typedef struct __attribute__((packed)) {
    uint64_t timestamp;
    power_module_telemetry_t data;
} timed_power_module_telemetry_t;

typedef struct __attribute__((packed)) {
    uint64_t timestamp;
    float data;
} timed_adc_telemetry_t;

/**
 * Initialize networking
 */
void init_networking();

/**
 * Task for reading INA219 sensor data into a queue
 */
void ina_task(void);

/**
 * Task for reading ADC data into a queue
 */
void adc_task(void);

/**
 * Task for broadcasting data from a queue over UDP
 */
void telemetry_broadcast_task(void);

#endif //POWER_MODULE_H
