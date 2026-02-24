#pragma once
#include "common.hpp"

namespace NModel {

float AltitudeMetersFromPressureKPa(float kPa);

void FeedKalman(uint64_t usSinceBoot, float verticalAccelerationMS2, float altitudeMeters);
KalmanState LastKalmanState();

struct GyroState {
    float angleOffInitial;
    float angleUncertainty;
};

void FeedGyro(uint64_t usSinceBoot, const NTypes::GyroscopeData &gyro);
int GetOrientation();
bool EverWentOutOfBounds();

float CalcActuatorEffort(float altitude, float velocity);
} // namespace NModel