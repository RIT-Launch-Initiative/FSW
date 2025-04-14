#ifndef N_GNSS_UTILS_H
#define N_GNSS_UTILS_H

#include <zephyr/drivers/gnss.h>

namespace NGnssUtils {
    static constexpr float nanodegreesToDegreesScale = 1e9f;
    static constexpr float millimeterToMeterScale = 1e3f;

    inline float NanodegreesToDegreesFloat(int64_t latitude) {
        return static_cast<float>(latitude) / nanodegreesToDegreesScale;
    }

    inline float MillimetersToMetersFloat(int64_t altitude) {
        return static_cast<float>(altitude) / millimeterToMeterScale;
    }

    inline double NanodegreesToDegreesDouble(int64_t latitude) {
        return static_cast<double>(latitude) / nanodegreesToDegreesScale;
    }

    inline float MillimetersToMetersDouble(int64_t altitude) {
        return static_cast<double>(altitude) / millimeterToMeterScale;
    }
};

#endif // N_GNSS_UTILS_H
