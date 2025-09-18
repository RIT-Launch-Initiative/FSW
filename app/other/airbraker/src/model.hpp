#include <cstdint>

// y in kalman filter
struct Measurement{
    float altitude_meas;
    float accel_meas;
};

struct Estimate{
    float altitude_est;
    float velocity_est;
    float accel_est;
    float accel_bias_est;
};

// maybe
struct KalmanOutput {
    Estimate est;
    float estimate_cov;
    float K_out;
};

float pressure_altitude(float pressure);


void init_kalman_filt();
KalmanOutput kalman_filt(const Measurement &meas);


float actuator_effort_lut(float altitude_est, float velocity_est);
