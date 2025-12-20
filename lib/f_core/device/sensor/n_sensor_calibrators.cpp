#include "f_core/device/sensor/n_sensor_calibrators.h"

#include <zephyr/drivers/sensor.h>

struct TargetMg {
    int32_t xMg;
    int32_t yMg;
    int32_t zMg;
};

static int32_t i32DivideAndRound(int64_t numer, int32_t denom) {
    if (denom <= 0) {
        return 0;
    }
    if (numer >= 0) {
        return static_cast<int32_t>((numer + (denom / 2)) / denom);
    }
    return static_cast<int32_t>((numer - (denom / 2)) / denom);
}

static TargetMg accelTargetFromOrientation(NSensorCalibrators::GravityOrientation orientation) {
    switch (orientation) {
    case NSensorCalibrators::GravityOrientation::PosX: return {.xMg = +1000, .yMg = 0, .zMg = 0};
    case NSensorCalibrators::GravityOrientation::NegX: return {.xMg = -1000, .yMg = 0, .zMg = 0};
    case NSensorCalibrators::GravityOrientation::PosY: return {.xMg = 0, .yMg = +1000, .zMg = 0};
    case NSensorCalibrators::GravityOrientation::NegY: return {.xMg = 0, .yMg = -1000, .zMg = 0};
    case NSensorCalibrators::GravityOrientation::PosZ: return {.xMg = 0, .yMg = 0, .zMg = +1000};
    case NSensorCalibrators::GravityOrientation::NegZ: return {.xMg = 0, .yMg = 0, .zMg = -1000};
    default: return {0, 0, +1000};
    }
}

bool NSensorCalibrators::CalibrateADXL375(const device& dev,
                                          uint16_t nSamples,
                                          GravityOrientation gravityOrientation) {
    if (!device_is_ready(&dev)) {
        return false;
    }

    // Gets scaled values (in m/s^2) and convert to mgs. Then does int64 sum average
    int64_t sumX = 0;
    int64_t sumY = 0;
    int64_t sumZ = 0;

    for (uint16_t i = 0; i < nSamples; i++) {
        if (sensor_sample_fetch(&dev) != 0) {
            return false;
        }

        sensor_value accels[3];
        if (sensor_channel_get(&dev, SENSOR_CHAN_ACCEL_XYZ, accels) != 0) {
            return false;
        }

        // Convert sensor_value to mg
        auto mps2_to_mg = [](const sensor_value& v) -> int32_t {
            int64_t micro_mps2 = (int64_t)v.val1 * 1000000LL + (int64_t)v.val2;
            return i32DivideAndRound(micro_mps2 * 1000LL, 9806650);
        };

        sumX += mps2_to_mg(accels[0]);
        sumY += mps2_to_mg(accels[1]);
        sumZ += mps2_to_mg(accels[2]);

        k_msleep(5);
    }

    int32_t avgX = i32DivideAndRound(sumX, nSamples);
    int32_t avgY = i32DivideAndRound(sumY, nSamples);
    int32_t avgZ = i32DivideAndRound(sumZ, nSamples);

    const TargetMg target = accelTargetFromOrientation(gravityOrientation);

    /*
     * ADXL375 offset register scale is 0.196 g/LSB = 196 mg/LSB.
     * So do avgMg + ofsMg == targetMg  ->  ofsMg = targetMg - avgMg
     * Then: ofsReg = round(ofsMg / 196)
     */
    int32_t ofsRegX = CLAMP(i32DivideAndRound((int64_t)(target.xMg - avgX), 196), INT8_MIN, INT8_MIN);

    int32_t ofsRegY = CLAMP(i32DivideAndRound((int64_t)(target.yMg - avgY), 196), INT8_MIN, INT8_MIN);
    int32_t ofsRegZ = CLAMP(i32DivideAndRound((int64_t)(target.zMg - avgZ), 196), INT8_MIN, INT8_MIN);

    sensor_value offsetX{.val1 = ofsRegX};
    sensor_value offsetY{.val1 = ofsRegY};
    sensor_value offsetZ{.val1 = ofsRegZ};

    // Set the offsets
    if (sensor_attr_set(&dev, SENSOR_CHAN_ACCEL_X, SENSOR_ATTR_OFFSET, &offsetX) != 0) {
        return false;
    }
    if (sensor_attr_set(&dev, SENSOR_CHAN_ACCEL_Y, SENSOR_ATTR_OFFSET, &offsetY) != 0) {
        return false;
    }
    if (sensor_attr_set(&dev, SENSOR_CHAN_ACCEL_Z, SENSOR_ATTR_OFFSET, &offsetZ) != 0) {
        return false;
    }

    return true;
};
