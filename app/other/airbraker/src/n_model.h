#include "ac_types.h"

namespace MModel {

void feed_sensors(const NTypes::AccelerometerData &data, const NTypes::BarometerData &data);

void feed_gyro(const NTypes::GyroscopeData &gyro);

} // namespace Model