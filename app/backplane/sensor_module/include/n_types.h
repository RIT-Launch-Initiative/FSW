#ifndef N_TYPES_H
#define N_TYPES_H

#include <n_ac_types.h>

namespace NTypes {
struct __attribute__((packed)) SensorData {
    NTypes::BarometerData PrimaryBarometer;
    NTypes::BarometerData SecondaryBarometer;

    NTypes::AccelerometerData Acceleration;

    NTypes::AccelerometerData ImuAcceleration;
    NTypes::GyroscopeData ImuGyroscope;

    NTypes::MagnetometerData Magnetometer;

    NTypes::TemperatureData Temperature;
};

}

#endif //N_TYPES_H
