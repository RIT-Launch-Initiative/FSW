#ifndef C_ACCELEROMETER_DEVICE_H
#define C_ACCELEROMETER_DEVICE_H

#include "c_sensor_device.h"

class CAccelerometer : public CSensorDevice {
public:
    explicit CAccelerometer(const device& dev);

    bool UpdateSensorValue() override;

    sensor_value GetSensorValue(sensor_channel chan) override;

private:
    using CBase = CSensorDevice;

    typedef struct {
        sensor_value x;
        sensor_value y;
        sensor_value z;
    } accel_data;

    accel_data acceleration;
};



#endif //C_ACCELEROMETER_DEVICE_H
