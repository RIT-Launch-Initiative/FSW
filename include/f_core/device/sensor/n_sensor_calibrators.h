#ifndef N_CALIBRATORS_H
#define N_CALIBRATORS_H

#include <zephyr/device.h>

namespace NSensorCalibrators {
    typedef enum {
        PosX,
        NegX,
        PosY,
        NegY,
        PosZ,
        NegZ
    } GravityOrientation;

    bool CalibrateADXL375(const device& dev, uint16_t nSamples = 100, GravityOrientation gravityOrientation = POS_Z);
}

#endif //N_CALIBRATORS_H
