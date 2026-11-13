#pragma once

#include <zsl/orientation/quaternions.h>

namespace NMathUtils {
/**
 * @brief Calculate roll, pitch, and yaw from a quaternion.
 *
 * @param[in] inputQuat The input quaternion.
 * @param[out] roll Output roll angle in degrees.
 * @param[out] pitch Output pitch angle in degrees.
 * @param[out] yaw Output yaw angle in degrees.
 */
void CalculateRollPitchYaw(zsl_quat inputQuat, float& roll, float& pitch, float& yaw);
}; // namespace NMathUtils
