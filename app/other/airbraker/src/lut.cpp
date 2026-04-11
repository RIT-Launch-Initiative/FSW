#include "quantile_lut_data.h"

#include <cmath>
#include <cstdint>

namespace NModel {

static const float lower_bounds_lut[] = {LUT_LOWER_BOUNDS_INITIALIZER};
static const float upper_bounds_lut[] = {LUT_UPPER_BOUNDS_INITIALIZER};


//static const float lut_pressure_values[] = {0, 1, 2, 3, 4};

//#define LUT_PRESSURE_SIZE 10
//#define LUT_PRESSURE_MINX 0
//#define LUT_PRESSURE_MAXX 100

struct IndexParts {
    std::size_t whole; // The index of the LUT value just before this value
    float fraction;    // How far between LUT[whole] and LUT[whole+1] we are for interpolating
};


/*
IndexParts genAltitudeIndexParts(float value) {
    if (value < LUT_PRESSURE_MINX) {
        return {0, 0};
    }
    if (value > LUT_PRESSURE_MAXX) {
        // to keep common code path below, if we would be at the point of last element and element off the end of the array
        // this will return second to last, 1 which causes the lerping to calculate the last element in its entirety
        return {LUT_PRESSURE_SIZE - 2, 1};
    }

    float float_index = (LUT_PRESSURE_SIZE - 1) * (value - LUT_PRESSURE_MINX) / (LUT_PRESSURE_MAXX - LUT_PRESSURE_MINX);

    float whole_part = 0;
    float fractional_part = modff(float_index, &whole_part);

    if (whole_part >= lut_pressure_size - 1) {
        // to keep common code path below, if we would be at the point of last element and element off the end of the array
        // this will return second to last, 1 which causes the lerping to calculate the last element in its entirety
        return {lut_pressure_size - 2, 1};
    }

    return {(std::size_t) whole_part, fractional_part};
}


void altitudeLut(float pressure_kpa, float *alt_out){
    IndexParts index = genIndexParts(pressure_kpa);
    float previous = lower_bounds_lut[index.whole];
    float next = lower_bounds_lut[index.whole + 1];
    *alt_out = lerp(index.fraction, lowerPrevious, lowerNext);
}


*/


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
 *          if amt = 0, returns from
 *          if amt = 1, returns to
 * This does not do bounds checking (it extrapolates), it is your responsibility to not pass in
 * any value of amount outside of the suggested range (or be prepared to face the consequences)
 */
float lerp(float amount, float from, float to) { return (from * (1 - amount)) + (to * amount); }

/**
 * Find bounds in LUT based on altitude
 * @param[in] altitude_est estimated altitude
 * @param[out] lower the lower bound of the LUT at this altitude.
 * @param[out] upper the upper bound of the LUT at this altitude.
 */
void boundsLUT(float altitude_est, float *lower, float *upper) {
    IndexParts index = genIndexParts(altitude_est);
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
    float q = (velocity - z_hat_min) / (z_hat_max - z_hat_min);
    if (q > 1) {
        return 1;
    } else if (q < 0) {
        return 0;
    }
    return q;
}

} // namespace NModel
