#include "n_autocoder_types.h"

namespace Model {

struct KalmanModelOutputs {
    float altitude;
    float velocity;
};

KalmanModelOutputs feed_sensors(uint64_t us_since_boot, const NTypes::AccelerometerData &acc,
                                const NTypes::BarometerData &barom);

struct OrientationModelOutputs {
    float angle_off_initial;
    float angle_uncertainty;
};

// vector pointing in the direction of the nose of the rocket
void set_gyro_initial_orientation(float x, float y, float z);
OrientationModelOutputs feed_gyro(uint64_t us_since_boot, const NTypes::GyroscopeData &gyro);
} // namespace Model
