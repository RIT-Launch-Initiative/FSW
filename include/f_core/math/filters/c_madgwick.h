#ifndef C_MADGWICK_H
#define C_MADGWICK_H

#include <zephyr/kernel.h>
#include <zsl/zsl.h>
#include <zsl/orientation/orientation.h>

class CMadgwick {
public:
    /**
     * Constructor
     * @param frequencyHz Frequency in Hz for the filter
     */
    CMadgwick(zsl_real_t frequencyHz) : frequencyHz(frequencyHz) {
    }

    /**
     * Calibrate the beta term for the Madgwick filter using calibration matrices.
     * @param accel Pointer to calibration matrix for accelerometer samples
     * @param gyro Pointer to calibration matrix for gyroscope samples
     * @param mag Pointer to calibration matrix for magnetometer samples
     * @param incl Pointer to inclination angle in degrees
     * @return ZSL status code
     */
    int CalibrateBetaTerm(zsl_mtx* accel, zsl_mtx* gyro, zsl_mtx* mag, zsl_real_t* incl) {
        if (accel == nullptr || gyro == nullptr) {
            return -EINVAL; // Require at least accel and gyro
        }

        return zsl_fus_cal_madg(accel, gyro, mag, frequencyHz, incl, &cfg.beta);
    }

    /**
     * Initialize the Madgwick filter with the given frequency.
     * @return ZSL status code
     */
    int Initialize() {
        return zsl_fus_madg_init(static_cast<uint32_t>(frequencyHz), &cfg);
    }

    /**
     * Run the Madgwick filter update step. All sensor data must be fed before calling.
     * @param q_out Output quaternion (must be a valid pointer)
     * @return ZSL status code
     */
    int Update(zsl_quat* q_out) {
        if (!accelFed || !gyroFed) {
            return -EINVAL; // Require at least accel and gyro
        }
        int rc = zsl_fus_madg_feed(
            &accelData,
            magFed ? &magData : nullptr,
            &gyroData,
            inclinationFed ? &inclination : nullptr,
            q_out,
            &cfg
        );
        resetFedFlags();
        return rc;
    }

    /**
     * Set the beta parameter for the Madgwick filter.
     * @param beta Beta value
     */
    void SetBeta(zsl_real_t betaVal) {
        cfg.beta = betaVal;
    }

    /**
     * Feed the Madgwick filter with accelerometer data
     * @param x X-axis acceleration
     * @param y Y-axis acceleration
     * @param z Z-axis acceleration
     */
    void FeedAccel(const zsl_real_t x, const zsl_real_t y, const zsl_real_t z) {
        accelBuff[0] = x;
        accelBuff[1] = y;
        accelBuff[2] = z;
        accelFed = true;
    }

    /**
     * Feed the Madgwick filter with magnetometer data
     * @param x X-axis magnetic field
     * @param y Y-axis magnetic field
     * @param z Z-axis magnetic field
     */
    void FeedMagn(const zsl_real_t x, const zsl_real_t y, const zsl_real_t z) {
        magBuff[0] = x;
        magBuff[1] = y;
        magBuff[2] = z;
        magFed = true;
    }

    /**
     * Feed the Madgwick filter with gyroscope data
     * @param x X-axis angular velocity
     * @param y Y-axis angular velocity
     * @param z Z-axis angular velocity
     */
    void FeedGyro(const zsl_real_t x, const zsl_real_t y, const zsl_real_t z) {
        gyroBuff[0] = x;
        gyroBuff[1] = y;
        gyroBuff[2] = z;
        gyroFed = true;
    }

    /**
     * Feed the Madgwick filter with Earth's magnetic field inclination angle
     * @param inc Inclination angle in degrees
     */
    void FeedInclination(const zsl_real_t inc) {
        inclination = inc;
        inclinationFed = true;
    }

private:
    // Configuration
    zsl_real_t frequencyHz;

    struct zsl_fus_madg_cfg {
        zsl_real_t beta = -1.0F;
    } cfg;

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
