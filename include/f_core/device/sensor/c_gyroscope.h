#pragma once

#include "c_sensor_device.h"

class CGyroscope : public CSensorDevice {
public:
    /**
     * Constructor
     * @param[in] dev Zephyr device structure
     */
    explicit CGyroscope(const device& dev);

    /**
     * See parent docs
     */
    bool UpdateSensorValue() override;

    /**
     * See parent docs
     */
    sensor_value GetSensorValue(sensor_channel chan) const override;

private:
    /**
     * Destructor
     */
    using CBase = CSensorDevice;

    typedef struct {
        sensor_value x;
        sensor_value y;
        sensor_value z;
    } SGyroscopeData;

    SGyroscopeData gyroscopeData;
};




