#ifndef OPENROCKET_IMU_H
#define OPENROCKET_IMU_H

struct or_data_t {
    float time_s;
#ifdef CONFIG_OPENROCKET_IMU
    float vert_accel;
    float lat_accel;

    float roll;
    float pitch;
    float yaw;
    // If we want to support magnetometer, can use  Vertical Orientation (zenith), Lateral Orientation (azimuth)
#endif
#ifdef CONFIG_OPENROCKET_BAROMETER
    float temperature;
    float pressure;
#endif
#ifdef CONFIG_OPENROCKET_GNSS
    float latitude;
    float longitude;
    float altitude;
#endif
};

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
    float time_s;
    enum or_event_t event;
};

#endif