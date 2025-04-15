#ifndef N_GNSS_UTILS_H
#define N_GNSS_UTILS_H

#include <zephyr/drivers/gnss.h>

namespace NGnssUtils {
    static constexpr float nanodegreesToDegreesScale = 1e9f;
    static constexpr float millimeterToMeterScale = 1e3f;

    inline float NanodegreesToDegreesFloat(const int64_t latitude) {
        return static_cast<float>(latitude) / nanodegreesToDegreesScale;
    }

    inline float MillimetersToMetersFloat(const int64_t altitude) {
        return static_cast<float>(altitude) / millimeterToMeterScale;
    }

    inline double NanodegreesToDegreesDouble(const int64_t latitude) {
        return static_cast<double>(latitude) / static_cast<double>(nanodegreesToDegreesScale);
    }

    inline double MillimetersToMetersDouble(const int64_t altitude) {
        return static_cast<double>(altitude) / static_cast<double>(millimeterToMeterScale);
    }
};

#endif // N_GNSS_UTILS_H
