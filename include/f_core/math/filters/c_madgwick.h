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
    CMadgwick(zsl_real_t frequencyHz);

    /**
     * Calibrate the beta term for the Madgwick filter using calibration matrices.
     * @param[in] accel Calibration matrix for accelerometer samples
     * @param[in] gyro Calibration matrix for gyroscope samples
     * @param[in] mag Calibration matrix for magnetometer samples
     * @param[in] incl Inclination angle in degrees (pointer)
     * @return ZSL status code
     */
    int CalibrateBetaTerm(zsl_mtx* accel, zsl_mtx* gyro, zsl_mtx* mag, zsl_real_t* incl);

    /**
     * Initialize the Madgwick filter with the given frequency.
     * @return ZSL status code
     */
    int Initialize();

    /**
     * Run the Madgwick filter update step. All sensor data must be fed before calling.
     * @param[out] quatOut Output quaternion
     * @return ZSL status code
     */
    int Update(zsl_quat& quatOut);

    /**
     * Set the beta parameter for the Madgwick filter.
     * @param beta Beta value
     */
    void SetBeta(zsl_real_t betaVal);

    /**
     * Feed the Madgwick filter with accelerometer data
     * @param x X-axis acceleration
     * @param y Y-axis acceleration
     * @param z Z-axis acceleration
     */
    void FeedAccel(const zsl_real_t x, const zsl_real_t y, const zsl_real_t z);
    /**
     * Feed the Madgwick filter with magnetometer data
     * @param x X-axis magnetic field
     * @param y Y-axis magnetic field
     * @param z Z-axis magnetic field
     */
    void FeedMagn(const zsl_real_t x, const zsl_real_t y, const zsl_real_t z);
    /**
     * Feed the Madgwick filter with gyroscope data
     * @param x X-axis angular velocity
     * @param y Y-axis angular velocity
     * @param z Z-axis angular velocity
     */
    void FeedGyro(const zsl_real_t x, const zsl_real_t y, const zsl_real_t z);
    /**
     * Feed the Madgwick filter with Earth's magnetic field inclination angle
     * @param inc Inclination angle in degrees
     */
    void FeedInclination(const zsl_real_t inc);

private:
    // Configuration
    zsl_real_t frequencyHz;

    struct zsl_fus_madg_cfg {
        zsl_real_t beta = 0.033; // AHRS states 0.033 for IMU and 0.041 for MARG
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
    void resetFedFlags();
};

#endif //C_MADGWICK_H
