#ifndef N_MATH_UTILS_H
#define N_MATH_UTILS_H

#include <zsl/zsl.h>

namespace NMathUtils {
    /**
     * @brief Calculate roll, pitch, and yaw from a quaternion.
     *
     * @param[in] inputQuat The input quaternion.
     * @param[out] roll Output roll angle in radians.
     * @param[out] pitch Output pitch angle in radians.
     * @param[out] yaw Output yaw angle in radians.
     */
    void CalculateRollPitchYaw(zsl_quat inputQuat, float& roll, float& pitch, float& yaw);
};

#endif //N_MATH_UTILS_H
