#pragma once
#include "n_autocoder_types.h"
#include "matrix.hpp"

namespace NModel {

using StateTransitionT = Matrix<4, 4>;
using StateT = Matrix<4, 1>;
using KalmanGainT = Matrix<4, 2>;
using KalmanOutputT = Matrix<2, 4>;


void FeedSensors(uint64_t usSinceBoot, const NTypes::AccelerometerData &adata, const NTypes::BarometerData &bdata);
void FeedGyro(uint64_t usSinceBoot, const NTypes::GyroscopeData &gyro);

} // namespace NModel