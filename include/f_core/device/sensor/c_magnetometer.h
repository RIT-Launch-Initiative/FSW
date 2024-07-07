#ifndef C_MAGNETOMETER_H
#define C_MAGNETOMETER_H

#include "c_sensor_device.h"
#include <zephyr/device.h>

class CMagnetometer : public CSensorDevice {
public:
    explicit CMagnetometer(const device& dev);

    bool UpdateSensorValue() override;

    sensor_value GetSensorValue(sensor_channel chan) override;

protected:
    ~CMagnetometer() = default;

private:
    using CBase = CSensorDevice;

    typedef struct {
        sensor_value x;
        sensor_value y;
        sensor_value z;
    } SMagnetometerData;

    SMagnetometerData magData;
};


#endif //C_MAGNETOMETER_H
