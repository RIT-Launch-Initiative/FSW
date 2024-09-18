#ifndef OPENROCKET_SENSORS_H
#define OPENROCKET_SENSORS_H

#include <zephyr/drivers/sensor.h>

#if CONFIG_OPENROCKET_SCALAR_TYPE_DOUBLE
typedef double or_scalar_t;
#else
typedef float or_scalar_t;
#endif

#define SCALE_OPENROCKET_NOISE(value) ((or_scalar_t) value / (or_scalar_t) 1000)

enum axis {
    OPENROCKET_AXIS_X,
    OPENROCKET_AXIS_Y,
    OPENROCKET_AXIS_Z,
};

enum gyro_axis {
    OPENROCKET_AXIS_ROLL,
    OPENROCKET_AXIS_PITCH,
    OPENROCKET_AXIS_YAW,
};

struct or_common_params {
    bool broken;                     // True if the sensor is broken and won't return data
    unsigned int sampling_period_us; // How often the sensor updates
    unsigned int lag_time_ms;        // How far behind 'real time' is this sensor measuring
    unsigned int measurement_us;     // How long does it take a sensor to read
};

/**
 * @brief Convert or_scalar_t (float or double) to zephyr sensor value
 * @param val sensor value ot fill in
 * @param inp the value to fill with
*/
int sensor_value_from_or_scalar(struct sensor_value* val, or_scalar_t inp);

/**
 * @brief Linear interpolate openrocket values
 * @param a value at t = 0
 * @param b value at t = 1
 * @param t interpolation value
 * @return value linear interpolated between a and b by t
 */
or_scalar_t or_lerp(or_scalar_t a, or_scalar_t b, or_scalar_t t);

/**
 * @brief Get the time to pass to the openrocket interpolator based on this device's specific parameters 
 * @param sampling_period_us the sample will be taken at the start of this period and stay the same throughout this period
 * @param lag_time_ms the sample will be delayed by this amount by the real amount 
 * @return the time (T+) used by openrocket to interpolate and get sensor values
 * This function is limited in precisison by CONFIG_SYS_CLOCK_TICKS_PER_SEC
 */
or_scalar_t or_get_time(const struct or_common_params* cfg);

/**
 * @brief Most times you go looking for a packet based on a time, its in between packets. 
 * @param last_lower_idx the low index from the last time you called this function (or 0 if you haven't called it yet)
 * @param or_time the time into the openrocket flight you want to view. (Lags, sampling rate, and launch delay should be computed before calling this function)
 * @param lower_idx out parameter of the index of the packet closest before the requested time
 * @param upper_idx out parameter of the index of the packet closest after the requested time
 * @param mix the mix between the two packets (0 if we are the low packet, 1 if we are upper packet). To be passed to or_lerp
 * If the time requested is before takeoff, both lower_idx and upper_idx will be 0
 * If the time requested is after the simulation ends, both lower_idx and upper_idx will be or_packets_size
 */
void or_find_bounding_packets(unsigned int last_lower_idx, or_scalar_t or_time, unsigned int* lower_idx,
                              unsigned int* upper_idx, or_scalar_t* mix);

/**
 * @brief Generate a random number in the range [-1, 1] for adding noise to sensors
 * if OPENROCKET_NOISE is not selected, this always returns 0
 */
or_scalar_t or_random(or_scalar_t magnitude);

struct or_data_t {
    or_scalar_t time_s; // s
#ifdef CONFIG_OPENROCKET_IMU
    or_scalar_t vert_accel; // m/s^2
    or_scalar_t lat_accel;  // m/s^2

    or_scalar_t roll;  // deg/s
    or_scalar_t pitch; // deg/s
    or_scalar_t yaw;   // deg/s
#endif
#ifdef CONFIG_OPENROCKET_BAROMETER
    or_scalar_t temperature; // °C
    or_scalar_t pressure;    // mbar
#endif
#ifdef CONFIG_OPENROCKET_GNSS
    or_scalar_t latitude;  // °
    or_scalar_t longitude; // °
    or_scalar_t velocity;  // m/s
    or_scalar_t altitude;  // m
    or_scalar_t bearing;   // °
#endif
#ifdef CONFIG_OPENROCKET_MAGNETOMETER
    or_scalar_t x; // Gauss
    or_scalar_t y; // Gauss
    or_scalar_t z; // Gauss

#endif
};

#ifdef CONFIG_OPENROCKET_EVENT_LOG
enum or_event_t {
    OR_EVENT_IGNITION,
    OR_EVENT_LAUNCH,
    OR_EVENT_LIFTOFF,
    OR_EVENT_LAUNCHROD,
    OR_EVENT_BURNOUT,
    OR_EVENT_EJECTION_CHARGE,
    OR_EVENT_APOGEE,
    OR_EVENT_RECOVERY_DEVICE_DEPLOYMENT,
    OR_EVENT_GROUND_HIT,
    OR_EVENT_SIMULATION_END,
};

struct or_event_occurance_t {
    or_scalar_t time_s;
    enum or_event_t event;
};
#endif

/**
 * @brief fill a packet with values before the simulation
 * @param packet the packet of data to fill in
 */
void or_get_presim(struct or_data_t* packet);

/**
 * @brief fill a packet with values after the simulation
 * @param packet the packet of data to fill in
 */
void or_get_postsim(struct or_data_t* packet);

#endif