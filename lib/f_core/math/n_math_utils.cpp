#include "f_core/math/n_math_utils.h"

void NMathUtils::CalculateRollPitchYaw(zsl_quat inputQuat, float& roll, float& pitch, float& yaw) {
    static constexpr zsl_real_t radiansToDegrees = 180.0 / ZSL_PI;
    zsl_euler euler{};

    zsl_quat_to_euler(&inputQuat, &euler);

    roll = static_cast<float>(euler.x * radiansToDegrees);
    pitch = static_cast<float>(euler.y * radiansToDegrees);
    yaw = static_cast<float>(euler.z * radiansToDegrees);
}
