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

void FeedGyro(uint64_t usSinceBoot, const NTypes::GyroscopeData &gyro) {}
int GetOrientation() { return 0; }
bool gyroOutOfBounds() { return false; }
bool EverWentOutOfBounds() { return everWentOutOfBounds; }

} // namespace NModel
