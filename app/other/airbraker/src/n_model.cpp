#include "n_model.h"

#include "math/matrix.hpp"
#include "n_model.h"

#include "math/matrix.hpp"
#include "n_autocoder_types.h"
#include "n_model.hpp"

#include <cmath>

namespace NModel {

using StateTransitionT = Matrix<4, 4>;

// clang-format off

const StateTransitionT state_transition_matrix{{ // aka F
    1,    0.01,    0.00005, 0,
    0,    1,       0,       0,
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

Matrix<4, 1> kalman_state({1.0,10.0,16.9,-9.8});

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

void FeedKalman(uint64_t usSinceBoot, float verticalAccelerationMS2, float altitudeMeters) {
    kalman_state = kalmanPredictAndUpdate(kalman_state, verticalAccelerationMS2, altitudeMeters);
}
KalmanState LastKalmanState() {
    return {
        .estAltitude = kalman_state.Get(0, 0),
        .estVelocity = kalman_state.Get(1, 0),
        .estAcceleration = kalman_state.Get(2, 0),
    };
}

} // namespace NModel
