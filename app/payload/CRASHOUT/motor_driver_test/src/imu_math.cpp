#include "common.hpp"
#include <zephyr/kernel.h>

#include <cmath>

// int16 vectors should not exceed this
constexpr int16_t max_16 = 32767;
int16_t saturate_i16(int32_t v) {
    if (v > max_16) {
        return max_16;
    } else if (v < -max_16) {
        return -max_16;
    }
    return v;
}

/** 
* @brief Integer square root for RMS 
* @param sqrtAvg (sum(x squared) / count) 
* @retval approximate square root 
* 
* Approximate integer square root, used for RMS calculations. 
* https://community.st.com/t5/stm32-mcus-products/fast-sqrt-function/td-p/515126
*/
static unsigned int sqrtI(unsigned long sqrtArg) {
    unsigned int answer = 0;
    unsigned int x = 0;
    unsigned long temp = 0;
    if (sqrtArg == 0) return 0;                   // undefined result
    if (sqrtArg == 1) return 1;                   // identity
    answer = 0;                                   // integer square root
    for (x = 0x8000; x > 0; x = x >> 1) {         // 16 bit shift
        answer |= x;                              // possible bit in root
        temp = ((unsigned long) answer * answer); // fast unsigned multiply
        if (temp == sqrtArg) break;               // exact, found it
        if (temp > sqrtArg) answer ^= x;          // too large, reverse bit
    }
    return answer; // approximate root
}

// return a vec32 with a max size of 2^15-1
Vec3_32 normalize_to16(Vec3_32 v) {
    uint64_t norm_squared = (((int64_t)v.x * v.x) + ((int64_t)v.y * v.y) + ((int64_t)v.z * v.z));
    uint32_t norm = sqrtI(norm_squared);

    // normed = -1 to 1
    // normed 16 = -2^15-1 to 2^15-1
    // normed = v/|v|
    // normed16 = (2^15-1)v/|v|
    int32_t x = (int32_t) (((int64_t)v.x * max_16) / (int32_t)norm);
    int32_t y = (int32_t) (((int64_t)v.y * max_16) / (int32_t)norm);
    int32_t z = (int32_t) (((int64_t)v.z * max_16) / (int32_t)norm);
    // shouldn't exceed, but just in case
    return {saturate_i16(x), saturate_i16(y), saturate_i16(z)};
}

// a and b absolutely have to be normalized or else this will saturate
int16_t dot_normed16(Vec3_32 a_normed16, Vec3_32 b_normed16) {
    int32_t xcontrib = ((int32_t) a_normed16.x * b_normed16.x) / max_16;
    int32_t ycontrib = ((int32_t) a_normed16.y * b_normed16.y) / max_16;
    int32_t zcontrib = ((int32_t) a_normed16.z * b_normed16.z) / max_16;
    // bc you promised the inputs are normalized, this should be at the most extreme -1 or 1 which is -max_16 or max_16. just incase, saturate (tho youll be wrong but at least it wont wrap)
    int32_t n = xcontrib + ycontrib + zcontrib;
    return saturate_i16(n);
}

constexpr int16_t imu_bad_thresh_cos_space = 26841; // cos 35 ~=0.819152044289, * max_16 = 29696

// false if we're measuring gravity along the axis that just isn't helpful
bool shouldntUseAccel(Vec3_32 measured_normed16) {
    constexpr Vec3_32 rotation_axis_n16{0, max_16, 0};
    int16_t n = dot_normed16(measured_normed16, rotation_axis_n16);
    return std::abs(n) > imu_bad_thresh_cos_space;
}

Vec3_32 cross_n16(Vec3_32 a_n16, Vec3_32 b_n16) {
    int32_t x = (a_n16.y * b_n16.z) - (a_n16.z * b_n16.y);
    int32_t y = -((a_n16.x * b_n16.z) - (a_n16.z * b_n16.x));
    int32_t z = (a_n16.x * b_n16.y) - (a_n16.y * b_n16.x);
    // since both inputs or normalized, the maginitude of output should be <=1 (<=max_16)
    return {x / max_16, y / max_16, z / max_16};
}

Vec3_32 rotate_by_udeg_about_z(Vec3_32 vec, int64_t udeg){
    float rad = ((float)udeg)*3.14159F/180.0F / 1000000.0F;
    float c = std::cos(rad);
    float s = std::sin(rad);
    float nx = vec.x * c - s * vec.y;
    float ny = vec.x * s + c * vec.y;
    return {saturate_i16(nx), saturate_i16(ny), vec.z};
}

// return false if theyre aligned and we cant get gravity
bool getVerticalAngleFromImus(Vec3_16 base_imu16_normalized, Vec3_32 link_imu, int64_t yaw, int64_t shoulder_pitch, int64_t *microdeg_out) {
    // just need to change datatypes
    Vec3_32 base_imu32_normed_16{base_imu16_normalized.x, base_imu16_normalized.y, base_imu16_normalized.z};

    Vec3_32 up_normed_16{0, 0, max_16};
    // need to actualy normalize here
    Vec3_32 link_imu32_normed_16 = normalize_to16(link_imu);
    if (shouldntUseAccel(link_imu32_normed_16)) {
        return false;
    }

    Vec3_32 base_imu32_normed_16_rotated = rotate_by_udeg_about_z(base_imu32_normed_16, -yaw);

    Vec3_32 cross = cross_n16(base_imu32_normed_16, link_imu32_normed_16);

    int16_t norm = dot_normed16(base_imu32_normed_16, link_imu32_normed_16);
    float normedf = norm / (float) max_16;
    float rad = std::acosf(normedf);
    int64_t microdeg_to_vertical = (rad * 180.0F / 3.14159F) * 1000000;
    if (cross.y >= 0) {
        microdeg_to_vertical *= -1;
    }
    *microdeg_out = microdeg_to_vertical - shoulder_pitch;

    return true;
}