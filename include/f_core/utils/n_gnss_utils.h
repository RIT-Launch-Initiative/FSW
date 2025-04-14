#ifndef N_GNSS_UTILS_H
#define N_GNSS_UTILS_H

#include <zephyr/drivers/gnss.h>

namespace NGnssUtils {
    static constexpr float zephyrCoordinateScale = 1e9f;
    static constexpr float zephyrAltitudeScale = 1e3f;

    inline float ScaleLatitudeInt64ToFloat(int64_t latitude) {
        return static_cast<float>(latitude) / zephyrCoordinateScale;
    }

    inline float ScaleLongitudeInt64ToFloat(int64_t longitude) {
        return static_cast<float>(longitude) / zephyrCoordinateScale;
    }

    inline float ScaleAltitudeInt64ToFloat(int64_t altitude) {
        return static_cast<float>(altitude) / zephyrAltitudeScale;
    }

    inline float ScaleLatitudeInt64ToDouble(int64_t latitude) {
        return static_cast<double>(latitude) / zephyrCoordinateScale;
    }

    inline float ScaleLongitudeInt64ToDouble(int64_t longitude) {
        return static_cast<double>(longitude) / zephyrCoordinateScale;
    }

    inline float ScaleAltitudeInt64ToDouble(int64_t altitude) {
        return static_cast<double>(altitude) / zephyrAltitudeScale;
    }
};

#endif // N_GNSS_UTILS_H
