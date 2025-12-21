#ifndef N_CALIBRATORS_H
#define N_CALIBRATORS_H

#include <zephyr/device.h>

namespace NSensorCalibrators {
    typedef enum {
        PosX = 0,
        NegX,
        PosY,
        NegY,
        PosZ,
        NegZ
    } GravityOrientation;

    bool CalibrateADXL375(CAccelerometer& accelerometer, uint16_t nSamples = 100,
                                GravityOrientation gravityOrientation = PosZ);
}

#endif //N_CALIBRATORS_H
