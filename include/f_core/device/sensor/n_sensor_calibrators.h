#pragma once

#include <zephyr/device.h>
#include <f_core/device/sensor/c_accelerometer.h>

namespace NSensorCalibrators {
typedef enum { PosX = 0, NegX, PosY, NegY, PosZ, NegZ } GravityOrientation;

bool CalibrateADXL375(CAccelerometer& accelerometer, uint16_t nSamples = 100,
                      GravityOrientation gravityOrientation = PosZ);
} // namespace NSensorCalibrators
