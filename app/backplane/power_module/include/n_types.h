#ifndef N_TYPES_H
#define N_TYPES_H
#include <f_core/types.h>

namespace NTypes {
struct __attribute__((packed)) SensorData {
    NSensor::ShuntData RailBattery;
    NSensor::ShuntData Rail3v3;
    NSensor::ShuntData Rail5v0;
};
}

#endif //N_TYPES_H
