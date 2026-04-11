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
void AltitudeLut(float pressure__kPa, float *altitudeM);
float AltitudeMetersFromPressureKPa(float kPa)
{
  float x = kPa * 1000;
  float y = 0;
  AltitudeLut(x, &y);
  return y;
}

/*
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
*/
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
  w_1 *= t;
  w_2 *= t;
  w_3 *= t;

  float w_1² = w_1 * w_1;
  float w_2² = w_2 * w_2;
  float w_3² = w_3 * w_3;
  float w_1w_2 = w_1 * w_2;
  float w_1w_3 = w_1 * w_3;
  float w_2w_3 = w_2 * w_3;
  // clang-format off
        Matrix<3,3> A{{
            0,    -w_3,    w_2, 
            w_3,    0,    -w_1, 
            -w_2,    w_1,     0,
        }};
        Matrix<3,3> A²{{
            -(w_2² + w_3²),    w_1w_2,           w_1w_3,
            w_1w_2,         -(w_1²+w_3²),       w_2w_3,
            w_1w_3,           w_2w_3,         -(w_1²+w_2²),
        }};

  // clang-format on
  float norm_sqred = w_1² + w_2² + w_3²;
  float norm = std::sqrt(norm_sqred);

  // proof via desmos, this is what happens. (sinx/x = 1   (1-cosx)/x = 0 )
  float s = (norm == 0) ? 1 : (std::sin(norm) / norm);
  float c = (norm == 0) ? 0 : ((1 - std::cos(norm)) / (norm_sqred));

  Matrix<3, 3> I = Matrix<3, 3>::Identity();

  auto eᴬᵗ = I + A * s + A² * c;
  return eᴬᵗ;
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
  Matrix<3, 1> nowInIMUSpace = gyroOrientation * initial;

  // for debugging only
  // NTypes::AccelerometerData rotatedUsInRocketSpace{ 0, 0, 1 };
  // RotateIMUVectorToRocketVector({ nowInIMUSpace.Get(0, 0), nowInIMUSpace.Get(1, 0), nowInIMUSpace.Get(2, 0) }, rotatedUsInRocketSpace);
  // end for debugging only
  
  auto dot = [](const Matrix<3, 1>& a, const Matrix<3, 1>& b) {
    return a.Get(0, 0) * b.Get(0, 0) + a.Get(1, 0) * b.Get(1, 0) + a.Get(2, 0) * b.Get(2, 0);
  };

  float normOfNow = std::sqrt(nowInIMUSpace.Get(0,0)*nowInIMUSpace.Get(0,0) + nowInIMUSpace.Get(1,0)*nowInIMUSpace.Get(1,0) + nowInIMUSpace.Get(2,0)*nowInIMUSpace.Get(2,0));
  Matrix<3, 1> normedNow = nowInIMUSpace * (1.F / normOfNow);


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
