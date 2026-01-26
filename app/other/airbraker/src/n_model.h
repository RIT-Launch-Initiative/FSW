#pragma once
#include "n_autocoder_types.h"
#include "math/matrix.hpp"

namespace NModel {


struct KalmanState {
    float estAltitude;
    float estVelocity;
    float estAcceleration;
};

void FeedKalman(uint64_t usSinceBoot, const NTypes::AccelerometerData &adata, const NTypes::BarometerData &bdata);
KalmanState LastKalmanState();

struct GyroState{
    float angleOffInitial;
    float angleUncertainty;
};

void FeedGyro(uint64_t usSinceBoot, const NTypes::GyroscopeData &gyro);
float CalculateEffort(const KalmanState &state);

} // namespace NModel