#ifndef N_CALIBRATORS_H
#define N_CALIBRATORS_H

namespace NSensorCalibrators {
    typedef enum {
        GRAVITY_ORIENTATION_POS_X,
        GRAVITY_ORIENTATION_NEG_X,
        GRAVITY_ORIENTATION_POS_Y,
        GRAVITY_ORIENTATION_NEG_Y,
        GRAVITY_ORIENTATION_POS_Z,
        GRAVITY_ORIENTATION_NEG_Z
    } GravityOrientation;

    bool CalibrateADXL375(const device &dev, uint16_t nSamples = 100, GravityOrientation orientation = GRAVITY_ORIENTATION_POS_Z);
}

#endif //N_CALIBRATORS_H