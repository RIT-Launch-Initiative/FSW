#include "n_model.hpp"

#include "math/matrix.hpp"
#include "n_autocoder_types.h"
#include "quantile_lut_data.h"
#include <cmath>

namespace NModel {
const char *GetMatlabLUTName() { return LUT_NAME; }
const char *GetMatlabLUTDate() { return LUT_CREATION_DATE; }

using StateTransitionT = Matrix<4, 4>;

const StateTransitionT state_transition_matrix{{ // aka F
    KALMAN_STATE_TRANSITION_INITIALIZER
}};

const Matrix<2, 4> kalman_output_matrix{{ // aka H
    KALMAN_OUTPUT_INITIALIZER
}};
const Matrix<4, 2> kalman_gain{{
    KALMAN_GAIN_INITIALIZER
}};

static Matrix<4, 1> kalman_state({KALMAN_INITIAL_STATE_INITIALIZER});

static Matrix<3,3> gyroOrientation = Matrix<3,3>::Identity();
static bool everWentOutOfBounds = false;


Matrix<4, 1> kalmanPredictAndUpdate(const Matrix<4, 1> &state, const float altitudeMeters,
                                    const float verticalAccelerationMS2) {
    Matrix<2, 1> sensorIn = Matrix<2, 1>::Column({altitudeMeters, verticalAccelerationMS2});

    // change via physics model
    Matrix<4, 1> fx = state_transition_matrix * state;

    // change via sensors
    Matrix<2, 1> innovation = sensorIn - (kalman_output_matrix * fx);
    Matrix<4, 1> correction = kalman_gain * innovation;

    return fx + correction;
}

void FeedKalman(uint64_t usSinceBoot, float altitudeMeters, float verticalAccelerationMS2) {

    kalman_state = kalmanPredictAndUpdate(kalman_state, altitudeMeters, verticalAccelerationMS2);
}
KalmanState LastKalmanState() {
    return {
        .estAltitude = kalman_state.Get(0, 0),
        .estVelocity = kalman_state.Get(1, 0),
        .estAcceleration = kalman_state.Get(2, 0),
        .estBias = kalman_state.Get(3, 0),
    };
}

float AltitudeMetersFromPressureKPa(float pressure_kpa) {
    float pressure = pressure_kpa * 10;
    float altitude = (1 - powf(pressure / 1013.25F, 0.190284F)) * (float) (145366.45 * 0.3048);
    return altitude;
}

Matrix<3, 3> expGyro(float w_1, float w_2, float w_3, float t) {
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
    float normt = norm * t;

    float s = std::sin(normt) / norm;
    float c = (1 - std::cos(normt)) / (norm_sqred);

    Matrix<3, 3> I = Matrix<3, 3>::Identity();

    auto eᴬᵗ = I + A * s + A² * c;
    return eᴬᵗ;
}


float deg2rad(float d){
    return d / 180.0F * 3.14159F;
}
void FeedGyro(uint64_t usSinceBoot, const NTypes::GyroscopeData &gyro) {
    const float t = 0.01;
    Matrix<3,3> eAT = expGyro(gyro.X, gyro.Y, gyro.Z, t);
    gyroOrientation = gyroOrientation * eAT;

    // initial and startAxis are dependent on orientation_quat of imu
    Matrix<3,1> initial{{0,0,1}};
    Matrix<3,1> now = gyroOrientation * initial;

    // pre transposed
    Matrix<1,3> startAxis{{0,0,1}};

    
    float offStart = acos((startAxis * now).Get(0,0));
    bool outOfBounds = offStart > deg2rad(30);
    everWentOutOfBounds |= outOfBounds;
}

int GetOrientation() { return 0; }
bool EverWentOutOfBounds() { return everWentOutOfBounds; }

} // namespace NModel
