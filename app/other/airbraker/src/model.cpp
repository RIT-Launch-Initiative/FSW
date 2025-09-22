#include "model.hpp"

#include "model_data.inc"

#include <cmath>

class Vec4 {
    float parts[4];
};

struct index_parts {
    size_t whole;   // The index of the LUT value just before this value
    float fraction; // How far between LUT[whole] and LUT[whole+1] we are for interpolating
};

struct index_parts gen_index_parts(float value) {
    if (value < LUT_MINIMUM_X) {
        return {0, 0};
    }
    if (value > LUT_MAXIMUM_X) {
        return {LUT_SIZE - 1, 0};
    }

    float float_index = (LUT_SIZE - 1) * (value - LUT_MINIMUM_X) / (LUT_MAXIMUM_X - LUT_MINIMUM_X);

    float whole_part = 0;
    float fractional_part = modff(float_index, &whole_part);

    if (whole_part >= LUT_SIZE - 1) {
        // to keep common code path below, if we would be at the point of last element and element off the end of the array
        // this will return second to last, 1 which causes the lerping to calculate the last element in its entirety
        return {LUT_SIZE - 2, 1};
    }

    return {(size_t) whole_part, fractional_part};
}

float pressure_altitude(float pressure) {
    float alt = (1 - powf(pressure / 101325.0f, 0.190284f)) * 145366.45f * 0.3048f;
    return alt;
}
/**
 * Linearly interpolate between from and to
 * @param amt value between 0 and 1 inclusive. How far between the two values
 * @returns linearly interpolated between from and to depending on amt
 * @returns from if amt = 0
 * @returns to if amt = 1
 * This does not do bounds checking, it is your responsibility to not pass in
 * any value of amount outside of the suggested range (or be prepared to face the consequences)
 */
float lerp(float amount, float from, float to) { return from * (1 - amount) + to * amount; }

void bounds_lut(float altitude_est, float *lower, float *upper) {
    struct index_parts index = gen_index_parts(altitude_est);
    float lowerPrevious = lower_bounds_lut[index.whole];
    float lowerNext = lower_bounds_lut[index.whole + 1];
    float upperPrevious = upper_bounds_lut[index.whole];
    float upperNext = upper_bounds_lut[index.whole + 1];
    *lower = lerp(index.fraction, lowerPrevious, lowerNext);
    *upper = lerp(index.fraction, upperPrevious, upperNext);
}


float actuator_effort_lut(float altitude_est, float velocity_est){
    float z_hat_min = 0;
    float z_hat_max = 0;
    bounds_lut(altitude_est, &z_hat_min, &z_hat_max);
    float Q = (velocity_est - z_hat_min) / (z_hat_max - z_hat_min);
    if (Q > 1){
        return 1;
    } else if (Q < 0){
        return 0;
    }
    return Q;

}
