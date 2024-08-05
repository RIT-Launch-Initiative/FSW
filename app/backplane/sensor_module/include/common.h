#ifndef COMMON_H
#define COMMON_H

struct __attribute__((packed)) telemetry {
    float AccelerationX;
    float AccelerationY;
    float AccelerationZ;
    float GyroscopeX;
    float GyroscopeY;
    float GyroscopeZ;
    float Pressure;
    float Temperature;
};

#endif //COMMON_H
