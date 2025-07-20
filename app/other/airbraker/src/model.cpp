#include "model.hpp"
#include <cmath>

float pressure_altitude(float pressure) {
    float alt = (1 - powf(pressure / 101325.0f, 0.190284f)) * 145366.45f * 0.3048f;
    return alt;
}


void init_kalman_filt(){}
KalmanOutput kalman_filt(const Measurement &meas){
    return {0};
}

ActuatorValue actuator_lut(Estimate &est){
    return 0;
}