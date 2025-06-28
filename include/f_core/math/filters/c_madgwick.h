#ifndef C_MADGWICK_H
#define C_MADGWICK_H

#include <zsl/zsl.h>
#include <zsl/orientation/orientation.h>

class CMadgwick {
public:
    /**
     * Constructor
     * @param frequencyHz Frequency in Hz for the filter
     */
    CMadgwick(zsl_real_t frequencyHz) : frequencyHz(frequencyHz), beta(-1) {
    }

    /**
     * Initialize the beta term for the Madgwick filter
     * @return ZSL status code
     */
    int Initialize() {
        return zsl_fus_cal_madg(gyroData, accelData, magData, frequencyHz, &beta);
    }

    void FeedAccel(const zsl_real_t x, const zsl_real_t y, const zsl_real_t z) {
        accelBuff[0] = x;
        accelBuff[1] = y;
        accelBuff[2] = z;
        accelFed = true;
    }

    void FeedMagn(const zsl_real_t x, const zsl_real_t y, const zsl_real_t z) {
        magBuff[0] = x;
        magBuff[1] = y;
        magBuff[2] = z;
        magFed = true;
    }

    void FeedGyro(const zsl_real_t x, const zsl_real_t y, const zsl_real_t z) {
        gyroBuff[0] = x;
        gyroBuff[1] = y;
        gyroBuff[2] = z;
        gyroFed = true;
    }

    void FeedInclination(const zsl_real_t inc) {
        inclination = inc;
        inclinationFed = true;
    }

private:
    // Configuration
    zsl_real_t frequencyHz;
    zsl_real_t beta;

    // Inputs
    zsl_real_t accelBuff[3] = {0.0F, 0.0F, 0.0F};
    zsl_vec accelData{.sz = 3, .data = accelBuff};

    zsl_real_t magBuff[3] = {0.0F, 0.0F, 0.0F};
    zsl_vec magData{.sz = 3, .data = magBuff};

    zsl_real_t gyroBuff[3] = {0.0F, 0.0F, 0.0F};
    zsl_vec gyroData{.sz = 3, .data = gyroBuff};

    zsl_real_t inclination = 0.0F; // Earth's magnetic field inclination angle in degrees

    bool accelFed = false;
    bool magFed = false;
    bool gyroFed = false;
    bool inclinationFed = false;

    /**
     * Reset the flags indicating which sensors have been fed
     */
    void resetFedFlags() {
        accelFed = false;
        magFed = false;
        gyroFed = false;
        inclinationFed = false;
    }
};

#endif //C_MADGWICK_H
