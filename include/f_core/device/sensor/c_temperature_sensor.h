#ifndef C_TEMPERATURE_SENSOR_H
#define C_TEMPERATURE_SENSOR_H

#include "c_sensor_device.h"
#include <zephyr/device.h>

class CTemperatureSensor : public CSensorDevice {
public:
    explicit CTemperatureSensor(const ::device& device);

    bool UpdateSensorValue() override;

    sensor_value GetSensorValue(sensor_channel chan) override;

private:
    using CBase = CSensorDevice;
    sensor_value temperature{};
};

#endif //C_TEMPERATURE_SENSOR_H
