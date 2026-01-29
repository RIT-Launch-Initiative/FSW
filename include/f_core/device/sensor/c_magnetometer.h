#pragma once

#include "c_sensor_device.h"
#include <zephyr/device.h>

class CMagnetometer : public CSensorDevice {
public:
    /**
     * Constructor
     * @param dev Zephyr device structure
     */
    explicit CMagnetometer(const device& dev);
    /**
     * Destructor
     */
    ~CMagnetometer() = default;

    /**
     * See parent docs
     */
    bool UpdateSensorValue() override;

    /**
     * See parent docs
     */
    sensor_value GetSensorValue(sensor_channel chan) const override;


private:
    using CBase = CSensorDevice;

    typedef struct {
        sensor_value x;
        sensor_value y;
        sensor_value z;
    } SMagnetometerData;

    SMagnetometerData magData;
};



