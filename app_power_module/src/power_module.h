#ifndef POWER_MODULE_H
#define POWER_MODULE_H

typedef enum {
 GROUND_STATE = 0,
 FLIGHT_STATE,
} FLIGHT_STATES;

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
