#include "f_core/math/filters/c_madgwick.h"

CMadgwick::CMadgwick(zsl_real_t frequencyHz) : frequencyHz(frequencyHz) {
    zsl_fus_madg_init(static_cast<uint32_t>(frequencyHz), &cfg);
}

int CMadgwick::CalibrateBetaTerm(zsl_mtx* accel, zsl_mtx* gyro, zsl_mtx* mag, zsl_real_t* incl) {
    if (accel == nullptr || gyro == nullptr) {
        return -EINVAL;
    }
    return zsl_fus_cal_madg(accel, gyro, mag, frequencyHz, incl, &cfg.beta);
}

int CMadgwick::Update(zsl_quat& quatOut) {
    if (!accelFed || !gyroFed) {
        return -EINVAL;
    }
    int rc = zsl_fus_madg_feed(
        &accelData,
        magFed ? &magData : nullptr,
        &gyroData,
        inclinationFed ? &inclination : nullptr,
        &quatOut,
        &cfg
    );
    resetFedFlags();
    return rc;
}

void CMadgwick::SetBeta(zsl_real_t betaVal) {
    cfg.beta = betaVal;
}

void CMadgwick::FeedAccel(const zsl_real_t x, const zsl_real_t y, const zsl_real_t z) {
    accelBuff[0] = x;
    accelBuff[1] = y;
    accelBuff[2] = z;
    accelFed = true;
}

void CMadgwick::FeedMagn(const zsl_real_t x, const zsl_real_t y, const zsl_real_t z) {
    magBuff[0] = x;
    magBuff[1] = y;
    magBuff[2] = z;
    magFed = true;
}

void CMadgwick::FeedGyro(const zsl_real_t x, const zsl_real_t y, const zsl_real_t z) {
    gyroBuff[0] = x;
    gyroBuff[1] = y;
    gyroBuff[2] = z;
    gyroFed = true;
}

void CMadgwick::FeedInclination(const zsl_real_t inc) {
    inclination = inc;
    inclinationFed = true;
}

void CMadgwick::resetFedFlags() {
    accelFed = false;
    magFed = false;
    gyroFed = false;
    inclinationFed = false;
}

