#include "quantile_lut_data.h"

#include <cmath>
#include <cstdint>

namespace NModel {

static const float lower_bounds_lut[] = {LUT_LOWER_BOUNDS_INITIALIZER};
static const float upper_bounds_lut[] = {LUT_UPPER_BOUNDS_INITIALIZER};

struct IndexParts {
    std::size_t whole; // The index of the LUT value just before this value
    float fraction;    // How far between LUT[whole] and LUT[whole+1] we are for interpolating
};

IndexParts genIndexParts(float value) {
    if (value < LUT_MINIMUM_X) {
        return {0, 0};
    }
    if (value > LUT_MAXIMUM_X) {
        // to keep common code path below, if we would be at the point of last element and element off the end of the array
        // this will return second to last, 1 which causes the lerping to calculate the last element in its entirety
        return {LUT_SIZE - 2, 1};
    }

    float float_index = (LUT_SIZE - 1) * (value - LUT_MINIMUM_X) / (LUT_MAXIMUM_X - LUT_MINIMUM_X);

    float whole_part = 0;
    float fractional_part = modff(float_index, &whole_part);

    if (whole_part >= LUT_SIZE - 1) {
        // to keep common code path below, if we would be at the point of last element and element off the end of the array
        // this will return second to last, 1 which causes the lerping to calculate the last element in its entirety
        return {LUT_SIZE - 2, 1};
    }

    return {(std::size_t) whole_part, fractional_part};
}

/**
 * Linearly interpolate between from and to
 * @param amt value between 0 and 1 inclusive. How far between the two values
 * @returns linearly interpolated between from and to depending on amt
 * @returns from if amt = 0
 * @returns to if amt = 1
 * This does not do bounds checking (it extrapolates), it is your responsibility to not pass in
 * any value of amount outside of the suggested range (or be prepared to face the consequences)
 */
float lerp(float amount, float from, float to) { return (from * (1 - amount)) + (to * amount); }

void boundsLUT(float altitude_est, float *lower, float *upper) {
    struct IndexParts index = genIndexParts(altitude_est);
    float lowerPrevious = lower_bounds_lut[index.whole];
    float lowerNext = lower_bounds_lut[index.whole + 1];
    float upperPrevious = upper_bounds_lut[index.whole];
    float upperNext = upper_bounds_lut[index.whole + 1];
    *lower = lerp(index.fraction, lowerPrevious, lowerNext);
    *upper = lerp(index.fraction, upperPrevious, upperNext);
}

float CalcActuatorEffort(float altitude, float velocity) {
    float z_hat_min = 0;
    float z_hat_max = 0;
    boundsLUT(altitude, &z_hat_min, &z_hat_max);
    float Q = (velocity - z_hat_min) / (z_hat_max - z_hat_min);
    if (Q > 1) {
        return 1;
    } else if (Q < 0) {
        return 0;
    }
    return Q;
}

} // namespace NModel