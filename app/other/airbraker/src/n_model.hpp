#pragma once
#include "common.hpp"

namespace NModel {
// Return static string built into LUT definition from matlab
const char *GetMatlabLUTName();
const char *GetMatlabLUTDate();

float AltitudeMetersFromPressureKPa(float kPa);

void FeedKalman(float altitudeMeters, float verticalAccelerationMS2);
KalmanState LastKalmanState();

struct GyroState {
    float angleOffInitial;
    float angleUncertainty;
};

void FeedGyro(uint32_t msSinceBoot, const NTypes::GyroscopeData &gyro);
void FillPacketWithOrientationMatrix(float *arr);
void FillPacketWithKalmanInformation(float *inno, KalmanState &state);

int GetOrientation();
bool EverWentOutOfBounds();

float CalcActuatorEffort(float altitude, float velocity);
} // namespace NModel