#ifndef N_TYPES_H
#define N_TYPES_H

#include <f_core/types.h>

namespace NTypes {
struct __attribute__((packed)) SensorData {
    NSensor::BarometerData PrimaryBarometer;
    NSensor::BarometerData SecondaryBarometer;

    NSensor::AccelerometerData Acceleration;

    NSensor::AccelerometerData ImuAcceleration;
    NSensor::GyroscopeData ImuGyroscope;

    NSensor::MagnetometerData Magnetometer;

    NSensor::TemperatureData Temperature;
};

}

#endif //N_TYPES_H
