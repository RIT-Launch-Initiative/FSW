#ifndef C_ACCELEROMETER_DEVICE_H
#define C_ACCELEROMETER_DEVICE_H

#include "c_sensor_device.h"

class CAccelerometer : public CSensorDevice {
public:
    /**
     * Constructor
     * @param[in] dev Zephyr Device Structure
     */
    explicit CAccelerometer(const device& dev);

    /**
     * See parent docs
     */
    bool UpdateSensorValue() override;

    /**
     * See parent docs
     */
    sensor_value GetSensorValue(sensor_channel chan) override;

private:
    using CBase = CSensorDevice;

    typedef struct {
        sensor_value x;
        sensor_value y;
        sensor_value z;
    } SAccelerometerData;

    SAccelerometerData acceleration;
};



#endif //C_ACCELEROMETER_DEVICE_H
