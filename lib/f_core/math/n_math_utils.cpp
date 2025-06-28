#include "f_core/math/n_math_utils.h"


void NMathUtils::CalculateRollPitchYaw(zsl_quat inputQuat, float& roll, float& pitch, float& yaw)
{
    static constexpr multiplier = 180 / ZSL_PI;
    zsl_euler euler;

    zsl_quat_to_euler(&inputQuat, &euler);

    roll = euler.x * multiplier;
    pitch = euler.y * multiplier;
    yaw = euler.z * multiplier;
}