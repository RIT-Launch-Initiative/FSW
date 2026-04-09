#include "n_model.hpp"

#include "math/matrix.hpp"
#include "n_autocoder_types.h"
#include "quantile_lut_data.h"
#include "n_preboost.hpp"

#include <cmath>

namespace NModel
{
const char* GetMatlabLUTName()
{
  return LUT_NAME;
}
const char* GetMatlabLUTDate()
{
  return LUT_CREATION_DATE;
}

using StateTransitionT = Matrix<4, 4>;

const StateTransitionT state_transition_matrix{ { // aka F
						  KALMAN_STATE_TRANSITION_INITIALIZER } };

const Matrix<2, 4> kalman_output_matrix{ { // aka H
					   KALMAN_OUTPUT_INITIALIZER } };
const Matrix<4, 2> kalman_gain{ { KALMAN_GAIN_INITIALIZER } };

static Matrix<4, 1> kalman_state({ KALMAN_INITIAL_STATE_INITIALIZER });

static Matrix<2, 1> lastInnovation{ { 0, 0 } };
static Matrix<3, 3> gyroOrientation = Matrix<3, 3>::Identity();
static bool everWentOutOfBounds = false;

Matrix<4, 1> kalmanPredictAndUpdate(const Matrix<4, 1>& state, const float altitudeMeters,
				    const float verticalAccelerationMS2)
{
  Matrix<2, 1> sensorIn = Matrix<2, 1>::Column({ altitudeMeters, verticalAccelerationMS2 });

  // change via physics model
  Matrix<4, 1> fx = state_transition_matrix * state;

  // change via sensors
  Matrix<2, 1> innovation = sensorIn - (kalman_output_matrix * fx);
  lastInnovation = innovation;

  Matrix<4, 1> correction = kalman_gain * innovation;

  return fx + correction;
}

void FeedKalman(float altitudeMeters, float verticalAccelerationMS2)
{
  kalman_state = kalmanPredictAndUpdate(kalman_state, altitudeMeters, verticalAccelerationMS2);
}
KalmanState LastKalmanState()
{
  return {
    .estAltitude = kalman_state.Get(0, 0),
    .estVelocity = kalman_state.Get(1, 0),
    .estAcceleration = kalman_state.Get(2, 0),
    .estBias = kalman_state.Get(3, 0),
  };
}
#define CUSTOM_ATMOSPHERE 1
#ifdef CUSTOM_ATMOSPHERE
float AltitudeMetersFromPressureKPa(float kPa)
{
  float x = kPa * 1000;
  float sum = 0;
  float xn = x;
  for (size_t i = 1; i < AUTOGEN_ATMOSPHERE_NUM_COEFFECIENTS; i++)
  {
    const float coeff = ATMOSPHERE[i];
    sum += coeff * xn;
    xn *= x;
  }
  return sum + ATMOSPHERE[0];
}
#else
float AltitudeMetersFromPressureKPa(float pressure_kpa)
{
  float pressure = pressure_kpa * 10;
  float altitude = (1 - powf(pressure / 1013.25F, 0.190284F)) * (float)(145366.45 * 0.3048);
  return altitude;
}
#endif

Matrix<3, 3> expGyro(float w_1, float w_2, float w_3, float t)
{
  const float wx = w_1 * t;
  const float wy = w_2 * t;
  const float wz = w_3 * t;

  const float wx2 = wx * wx;
  const float wy2 = wy * wy;
  const float wz2 = wz * wz;
  const float wxwy = wx * wy;
  const float wxwz = wx * wz;
  const float wywz = wy * wz;

  const float thetaSq = wx2 + wy2 + wz2;
  const float theta = std::sqrt(thetaSq);
  const float s = (theta == 0.0F) ? 1.0F : (std::sin(theta) / theta);
  const float c = (theta == 0.0F) ? 0.0F : ((1.0F - std::cos(theta)) / thetaSq);

  return Matrix<3, 3>{ {
    1.0F - c * (wy2 + wz2), c * wxwy - s * wz, c * wxwz + s * wy,
    c * wxwy + s * wz, 1.0F - c * (wx2 + wz2), c * wywz - s * wx,
    c * wxwz - s * wy, c * wywz + s * wx, 1.0F - c * (wx2 + wy2),
  } };
}

float deg2rad(float d)
{
  return d / 180.0F * 3.14159F;
}
float rad2deg(float d)
{
  return d * 180.0F / 3.14159F;
}

static uint32_t lastGyroIntegrationMsSinceBoot = 0;
void FeedGyro(uint32_t msSinceBoot, const NTypes::GyroscopeData& gyro)
{
  float t = 0.01;
  if (lastGyroIntegrationMsSinceBoot != 0)
  {
    const int32_t deltaMs = (int32_t)(msSinceBoot - lastGyroIntegrationMsSinceBoot);
    t = static_cast<float>(deltaMs) / 1000.F;
  }
  lastGyroIntegrationMsSinceBoot = msSinceBoot;
  Matrix<3, 3> eAT = expGyro(gyro.X, gyro.Y, gyro.Z, t);
  gyroOrientation = gyroOrientation * eAT;
  NTypes::AccelerometerData zInIMUSpace;
  RotateRocketVectorToIMUVector({ 0, 0, 1 }, zInIMUSpace);
  // initial and startAxis are dependent on orientation_quat of imu

  Matrix<3, 1> initial{ { zInIMUSpace.X, zInIMUSpace.Y, zInIMUSpace.Z } };
  Matrix<3, 1> now = gyroOrientation * initial;
  NTypes::AccelerometerData rotatedUsInRocketSpace{ 0, 0, 1 };
  RotateIMUVectorToRocketVector({ now.Get(0, 0), now.Get(1, 0), now.Get(2, 0) }, rotatedUsInRocketSpace);

  // initial = rotate([0 0 1] by quaternion)
  //  now = rotate initial by gyroOrientation

  auto dot = [](const Matrix<3, 1>& a, const Matrix<3, 1>& b) {
    return a.Get(0, 0) * b.Get(0, 0) + a.Get(1, 0) * b.Get(1, 0) + a.Get(2, 0) * b.Get(2, 0);
  };

  float normOfNow = std::sqrt(initial.Get(0,0)*initial.Get(0,0) + initial.Get(1,0)*initial.Get(1,0) + initial.Get(2,0)*initial.Get(2,0));
  Matrix<3, 1> normedNow = initial * (1.F / normOfNow);
  float offStart = acos(dot(initial, normedNow));
  bool outOfBounds = offStart > deg2rad(30);
  everWentOutOfBounds |= outOfBounds;
}

int GetOrientation()
{
  return 0;
}
bool EverWentOutOfBounds()
{
  return everWentOutOfBounds;
}

void FillPacketWithOrientationMatrix(float* arr)
{
  for (size_t i = 0; i < 9; i++)
  {
    arr[i] = gyroOrientation.Get(i / 3, i % 3);
  }
}

void FillPacketWithKalmanInformation(float* inno, KalmanState &state)
{
  inno[0] = lastInnovation.Get(0, 0);
  inno[1] = lastInnovation.Get(1, 0);
  state = LastKalmanState();
}

}  // namespace NModel
