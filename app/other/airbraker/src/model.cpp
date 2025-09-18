#include "model.hpp"
#include <cmath>

class Vec4{
    float parts[4];
};


float pressure_altitude(float pressure) {
    float alt = (1 - powf(pressure / 101325.0f, 0.190284f)) * 145366.45f * 0.3048f;
    return alt;
}


void bounds_lut(float altitude_est, float *lower, float *upper){
    *lower = 0;
    *upper = 1;
}

float actuator_lut(float altitude_est, float velocity_est){
    float z_dot_min = 0;
    float z_dot_max = 0;

    float Q  = (velocity_est - z_dot_min) / (z_dot_max - z_dot_min);
    return Q;
}