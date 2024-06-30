#ifndef OPENROCKET_SENSORS_H
#define OPENROCKET_SENSORS_H

#if CONFIG_OPENROCKET_SCALAR_TYPE_DOUBLE
typedef double or_scalar_t;
#else
typedef float or_scalar_t;
#endif

/**
 * @brief Linear interpolate openrocket values
 * @param a value at t = 0
 * @param b value at t = 1
 * @param t interpolation value
 * @return value linear interpolated between a and b by t
 */
or_scalar_t or_lerp(or_scalar_t a, or_scalar_t b, or_scalar_t t);

struct or_data_t {
    or_scalar_t time_s;
#ifdef CONFIG_OPENROCKET_IMU
    or_scalar_t vert_accel;
    or_scalar_t lat_accel;

    or_scalar_t roll;
    or_scalar_t pitch;
    or_scalar_t yaw;
    // If we want to support magnetometer, can use  Vertical Orientation (zenith), Lateral Orientation (azimuth)
#endif
#ifdef CONFIG_OPENROCKET_BAROMETER
    or_scalar_t temperature;
    or_scalar_t pressure;
#endif
#ifdef CONFIG_OPENROCKET_GNSS
    or_scalar_t latitude;
    or_scalar_t longitude;
    or_scalar_t altitude;
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

#endif