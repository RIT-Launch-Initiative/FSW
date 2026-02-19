#pragma once
#include "n_autocoder_types.h"
#pragma once
#include "n_autocoder_types.h"

namespace NModel {

struct KalmanState {
    float estAltitude;
    float estVelocity;
    float estAcceleration;
};

void FeedKalman(uint64_t usSinceBoot, float verticalAccelerationMS2, float altitude_feet);
KalmanState LastKalmanState();

struct GyroState {
    float angleOffInitial;
    float angleUncertainty;
};

void FeedGyro(uint64_t usSinceBoot, const NTypes::GyroscopeData &gyro);

float CalcActuatorEffort(float altitude, float velocity);
} // namespace NModel