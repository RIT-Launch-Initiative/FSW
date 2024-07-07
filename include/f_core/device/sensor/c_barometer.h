#ifndef C_BAROMETER_DEVICE_H
#define C_BAROMETER_DEVICE_H

#include "c_sensor_device.h"
#include <zephyr/device.h>

class CBarometerDevice : public CSensorDevice {
public:

    explicit CBarometerDevice(const device& dev)
        : CSensorDevice(dev)
    {
    }

    sensor_value GetSensorValue(sensor_channel chan) override;
private:
    using CBase = CSensorDevice;

};



#endif //C_BAROMETER_DEVICE_H
