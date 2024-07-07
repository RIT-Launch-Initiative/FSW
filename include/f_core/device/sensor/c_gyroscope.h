#ifndef C_GYROSCOPE_H
#define C_GYROSCOPE_H

#include "c_sensor_device.h"

class CGyroscope : public CSensorDevice {
public:
    explicit CGyroscope(const device& dev);

    bool UpdateSensorValue() override;

    sensor_value GetSensorValue(sensor_channel chan) override;

private:
    using CBase = CSensorDevice;

    typedef struct {
        sensor_value x;
        sensor_value y;
        sensor_value z;
    } SGyroscopeData;

    SGyroscopeData gyroscopeData;
};



#endif //C_GYROSCOPE_H
