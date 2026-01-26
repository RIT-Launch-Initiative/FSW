#include "n_model.h"

#include "math/matrix.hpp"
#include "n_autocoder_types.h"

namespace NModel {
using StateTransitionT = Matrix<4, 4>;

// clang-format off

const StateTransitionT state_transition_matrix{{ // aka F
    1, 2, 3, 4,
    1, 2, 3, 4,
    1, 2, 3, 4,
    1, 2, 3, 4,
}};

const Matrix<2, 4> kalman_output_matrix{{
    1,2,3,4,
    1,2,3,4,
}};
const Matrix<4, 2> kalman_gain{{
    1,2,
    1,2,
    1,2,
    1,2
}};

// clang-format on

Matrix<4, 1> kalmanPredictAndUpdate(const StateT &state, const float altitude, const float acceleration) {
    Matrix<2, 1> sensor_in = Matrix<2, 1>::Column({altitude, acceleration});

    // change via physics model
    Matrix<4, 1> f_x = state_transition_matrix * state;

    // change via sensors
    Matrix<2, 1> innovation = sensor_in - (kalman_output_matrix * f_x);
    Matrix<4, 1> correction = kalman_gain * innovation;

    return f_x + correction;
}

} // namespace NModel
