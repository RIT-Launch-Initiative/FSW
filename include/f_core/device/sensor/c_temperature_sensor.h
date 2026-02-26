#pragma once

#include "c_sensor_device.h"
#include <zephyr/device.h>

class CTemperatureSensor : public CSensorDevice {
public:

    /**
     * Constructor
     * @param[in] dev Zephyr device structure
     */
    explicit CTemperatureSensor(const device& dev);

    /**
     * Destructor
     */
    ~CTemperatureSensor() = default;

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
    sensor_value temperature{};
};


