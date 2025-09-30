#include "model.hpp"

#include "model_data.inc"

#include <cmath>

struct vec2 {
    float s[2];
};
struct vec4 {
    float s[4];
};
struct mat4 {
    vec4 vs[4];
};
struct mat2 {
    vec2 vs[2];
};

mat2 mul_s_m2(float scalar, const mat2 *mat) {
    mat2 out = {0};
    out.vs[0].s[0] = mat->vs[0].s[0] * scalar;
    out.vs[0].s[1] = mat->vs[0].s[1] * scalar;
    out.vs[1].s[0] = mat->vs[1].s[0] * scalar;
    out.vs[1].s[1] = mat->vs[1].s[1] * scalar;
    return out;
}

// mat4 mul_m4_m4(const mat4 *a, const mat4 *b) {
    // mat4 out = {0};
    // for (size_t i = 0; i < 4; i++) {
        // for (size_t j = 0; j < 4; j++) {
            // for (size_t k = 0; k < 4; k++) {
                // out.vs[i].s[j] += a[i][j                
            // }
        // }
    // }
// }

bool inv2x2(const mat2 *in, mat2 *out) {
    float det = (*in).vs[0].s[0] * (*in).vs[1].s[1] - (*in).vs[0].s[1] * (*in).vs[1].s[0]; // ad -bc;
    if (det == 0) {
        return false;
    }
    mat2 temp = {0};
    temp.vs[0].s[0] = in->vs[1].s[1];
    temp.vs[1].s[1] = in->vs[0].s[0];
    temp.vs[0].s[1] = -in->vs[0].s[1];
    temp.vs[1].s[0] = -in->vs[1].s[0];

    *out = mul_s_m2((1 / det), &temp);
    return true;
}

struct index_parts {
    size_t whole;   // The index of the LUT value just before this value
    float fraction; // How far between LUT[whole] and LUT[whole+1] we are for interpolating
};

struct index_parts gen_index_parts(float value) {
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

float actuator_effort_lut(float altitude_est, float velocity_est) {
    float z_hat_min = 0;
    float z_hat_max = 0;
    bounds_lut(altitude_est, &z_hat_min, &z_hat_max);
    float Q = (velocity_est - z_hat_min) / (z_hat_max - z_hat_min);
    if (Q > 1) {
        return 1;
    } else if (Q < 0) {
        return 0;
    }
    return Q;
}
