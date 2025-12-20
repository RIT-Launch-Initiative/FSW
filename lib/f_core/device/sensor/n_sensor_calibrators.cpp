#include "f_core/device/sensor/c_accelerometer.h"
#include "f_core/device/sensor/n_sensor_calibrators.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(NSensorCalibrators);


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

bool NSensorCalibrators::CalibrateAccelerometer(CAccelerometer& accelerometer,
                                                uint16_t nSamples,
                                                GravityOrientation gravityOrientation) {
    if (!accelerometer.IsReady()) {
        LOG_ERR("Accelerometer not ready");
        return false;
    }

    // Reset offsets to 0 before calibration
    const sensor_value zeroOffset{.val1 = 0, .val2 = 0};
    int ret = accelerometer.Configure(SENSOR_CHAN_ACCEL_X, SENSOR_ATTR_OFFSET, &zeroOffset);
    if (ret != 0) {
        LOG_ERR("Failed to set accelerometer X offset to zero before calibration: %d", ret);
        return false;
    }
    ret = accelerometer.Configure(SENSOR_CHAN_ACCEL_Y, SENSOR_ATTR_OFFSET, &zeroOffset);
    if (ret != 0) {
        LOG_ERR("Failed to set accelerometer Y offset to zero before calibration: %d", ret);
        return false;
    }
    ret = accelerometer.Configure(SENSOR_CHAN_ACCEL_Z, SENSOR_ATTR_OFFSET, &zeroOffset);
    if (ret != 0) {
        LOG_ERR("Failed to set accelerometer Z offset to zero before calibration: %d", ret);
        return false;
    }

    // Gets scaled values (in m/s^2) and convert to mgs. Then does int64 sum average
    int64_t sumX = 0;
    int64_t sumY = 0;
    int64_t sumZ = 0;

    // Sleep a bit to let things settle
    k_msleep(5);

    for (uint16_t i = 0; i < nSamples; i++) {
        if (!accelerometer.UpdateSensorValue()) {
            LOG_WRN_ONCE("Failed to update accelerometer sensor value during calibration");
            continue;
        }

        sensor_value accels[3]{0};
        accels[0] = accelerometer.GetSensorValue(SENSOR_CHAN_ACCEL_X);
        accels[1] = accelerometer.GetSensorValue(SENSOR_CHAN_ACCEL_Y);
        accels[2] = accelerometer.GetSensorValue(SENSOR_CHAN_ACCEL_Z);

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
    LOG_INF("Accelerometer averages (mg): X=%d, Y=%d, Z=%d", avgX, avgY, avgZ);

    const TargetMg target = accelTargetFromOrientation(gravityOrientation);

    /*
     * ADXL375 offset register scale is 0.196 g/LSB = 196 mg/LSB.
     * So do avgMg + ofsMg == targetMg  ->  ofsMg = targetMg - avgMg
     * Then: ofsReg = round(ofsMg / 196)
     */
    int32_t ofsRegX = i32DivideAndRound((int64_t)(target.xMg - avgX), 196);

    int32_t ofsRegY = i32DivideAndRound((int64_t)(target.yMg - avgY), 196);
    int32_t ofsRegZ = i32DivideAndRound((int64_t)(target.zMg - avgZ), 196);

    LOG_INF("Pre-clamped offset registers: X=%d, Y=%d, Z=%d", ofsRegX, ofsRegY, ofsRegZ);

    ofsRegX = CLAMP(ofsRegX, INT8_MIN, INT8_MAX);
    ofsRegY = CLAMP(ofsRegY, INT8_MIN, INT8_MAX);
    ofsRegZ = CLAMP(ofsRegZ, INT8_MIN, INT8_MAX);

    sensor_value offsetX{.val1 = ofsRegX};
    sensor_value offsetY{.val1 = ofsRegY};
    sensor_value offsetZ{.val1 = ofsRegZ};

    // Set the offsets
    ret = accelerometer.Configure(SENSOR_CHAN_ACCEL_X, SENSOR_ATTR_OFFSET, &offsetX);
    if (ret != 0) {
        LOG_WRN("Failed to set accelerometer X offset during calibration: %d", ret);
    }

    ret = accelerometer.Configure(SENSOR_CHAN_ACCEL_Y, SENSOR_ATTR_OFFSET, &offsetY);
    if (ret != 0) {
        LOG_WRN("Failed to set accelerometer Y offset during calibration: %d", ret);
    }

    ret = accelerometer.Configure(SENSOR_CHAN_ACCEL_Z, SENSOR_ATTR_OFFSET, &offsetZ);
    if (ret != 0) {
        LOG_WRN("Failed to set accelerometer Z offset during calibration: %d", ret);
    }

    LOG_INF("Accelerometer calibration complete. Offsets set to X: %d, Y: %d, Z: %d", ofsRegX, ofsRegY, ofsRegZ);

    return true;
};
