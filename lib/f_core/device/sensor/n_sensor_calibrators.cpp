#include "f_core/device/sensor/n_sensor_calibrators.h"

#include <zephyr/drivers/sensor.h>

static inline int32_t div_round_i32(int64_t numer, int32_t denom) {
    if (denom <= 0) {
        return 0;
    }
    if (numer >= 0) {
        return static_cast<int32_t>((numer + (denom / 2)) / denom);
    }
    return static_cast<int32_t>((numer - (denom / 2)) / denom);
}

struct TargetMg {
    int32_t x_mg;
    int32_t y_mg;
    int32_t z_mg;
};

static inline TargetMg target_from_orientation(NSensorCalibrators::GravityOrientation o) {
    switch (o) {
    case NSensorCalibrators::GravityOrientation::PosX: return {.x_mg = +1000, .y_mg = 0, .z_mg = 0};
    case NSensorCalibrators::GravityOrientation::NegX: return {.x_mg = -1000, .y_mg = 0, .z_mg = 0};
    case NSensorCalibrators::GravityOrientation::PosY: return {.x_mg = 0, .y_mg = +1000, .z_mg = 0};
    case NSensorCalibrators::GravityOrientation::NegY: return {.x_mg = 0, .y_mg = -1000, .z_mg = 0};
    case NSensorCalibrators::GravityOrientation::PosZ: return {.x_mg = 0, .y_mg = 0, .z_mg = +1000};
    case NSensorCalibrators::GravityOrientation::NegZ: return {.x_mg = 0, .y_mg = 0, .z_mg = -1000};
    default: return {0, 0, +1000};
    }
}

bool NSensorCalibrators::CalibrateADXL375(const device& dev, uint16_t nSamples, GravityOrientation gravityOrientation) {
    if (!device_is_ready(&dev)) {
        return false;
    }

    sensor_value offsetX = {0, 0};
    sensor_value offsetY = {0, 0};
    sensor_value offsetZ = {0, 0};


    return true;
}
