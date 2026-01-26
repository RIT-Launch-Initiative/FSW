#include "n_model.h"
#include "matrix.hpp"
#include "n_autocoder_types.h"

namespace NModel {
constexpr StateTransitionT state_transition_matrix{
    std::array<std::array<double, 4>, 4>{std::array<double, 4>{1, 2, 3, 4}, std::array<double, 4>{1, 2, 3, 4},
                                        std::array<double, 4>{1, 2, 3, 4}, std::array<double, 4>{1, 2, 3, 4}}};

constexpr KalmanOutputT kalman_output_matrix = KalmanOutputT::Zeros();
constexpr KalmanGainT kalman_gain_matrix = KalmanGainT::Zeros();

StateT kalman_predict_and_update(const StateT &state, const float altitude, const float acceleration) {
    Matrix<2, 1> sensor_in = Matrix<2, 1>::Column({altitude, acceleration});

    Matrix<4, 1> f_x = state_transition_matrix * state;

    Matrix<2, 1> innovation = sensor_in - (kalman_output_matrix * f_x);

    Matrix<4, 1> correction = kalman_gain_matrix * innovation;

    return f_x + correction;
}

struct KalmanModelOutputs {
    float altitude;
    float velocity;
};

// KalmanModelOutputs FeedSensors(uint64_t usSinceBoot, const NTypes::AccelerometerData &acc,
//                                const NTypes::BarometerData &barom);

struct OrientationModelOutputs {
    float angle_off_initial;
    float angle_uncertainty;
};

// vector pointing in the direction of the nose of the rocket
// void SetGyroInitialOrientation(float x, float y, float z);
// OrientationModelOutputs FeedGyro(uint64_t usSinceBoot, const NTypes::GyroscopeData &gyro);
} // namespace NModel
