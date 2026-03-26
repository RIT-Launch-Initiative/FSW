#include "n_model.hpp"

#include "math/matrix.hpp"
#include "n_autocoder_types.h"

#include <cmath>

namespace NModel {
const char *GetMatlabLUTName() { return "AUTOGEN-FROM-MATLAB"; }

using StateTransitionT = Matrix<4, 4>;

// clang-format off

const StateTransitionT state_transition_matrix{{ // aka F
    1,    0.01,    5e-5,    0,
    0,    1,       0.01,    0,
    0,    0,       1,       0,
    0,    0,       0,       1,
}};

const Matrix<2, 4> kalman_output_matrix{{ // aka H
    1,0,0,0,
    0,0,1,1,
}};
const Matrix<4, 2> kalman_gain{{
    0.1989683408635,     1.0003523356894e-09,
    1.12897625945632,    2.48344519337973e-07,
    1.82817311684792,    0.967713284079819,
    -1.82817311684447,   0.032260907861791
}};

static Matrix<4, 1> kalman_state({0, 0,  0,  9.8});
static bool everWentOutOfBounds = false;

// clang-format on

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

void FeedGyro(uint64_t usSinceBoot, const NTypes::GyroscopeData &gyro) {}
int GetOrientation() { return 0; }
bool gyroOutOfBounds() { return false; }
bool EverWentOutOfBounds() { return everWentOutOfBounds; }

} // namespace NModel
