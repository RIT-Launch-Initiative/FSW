#ifndef POTATO_H
#define POTATO_H

#include <stdint.h>

#include <launch_core/types.h>

typedef enum
{
    PAD_STATE = 0,
    BOOST_STATE,
    COAST_STATE,
    APOGEE_STATE,
    MAIN_STATE,
    LANDING_STATE
} FLIGHT_STATES;

typedef struct __attribute__((packed))
{
    l_barometer_data_t lps22_data;
    float load;
    uint32_t timestamp;
} potato_raw_telemetry_t;

typedef struct __attribute__((packed))
{
    float altitude;
    float load;
    uint32_t timestamp;
} potato_telemetry_t;

typedef struct __attribute__((packed))
{
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

/**
 * Initialize a Modbus Server
 */
int init_modbus_server(void);

/**
 * Place a float into an input register
 * @param addr - Address of the register
 * @param value - Value to place in the register
 * @return 0 on success, negative error code on failure
 */
int insert_float_to_input_reg(uint16_t addr, float value);

#endif //POTATO_H
