#include "ac_types.h"

namespace MModel {

void FeedSensors(const NTypes::AccelerometerData &data, const NTypes::BarometerData &data);

void FeedGyro(const NTypes::GyroscopeData &gyro);

} // namespace Model