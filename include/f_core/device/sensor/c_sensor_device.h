#ifndef C_SENSOR_DEVICE_H
#define C_SENSOR_DEVICE_H

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

class CSensorDevice {
public:
    CSensorDevice(const device &device) : dev(device), isInitialized(false) {}

    bool Initialize() {
        isInitialized = device_is_ready(&dev);
        return isInitialized;
    }

    virtual bool UpdateSensorValue() {
        return (0 == sensor_sample_fetch(&dev));
    }

    virtual sensor_value GetSensorValue(sensor_channel chan);

    float GetSensorValueFloat(sensor_channel chan) {
        sensor_value val = GetSensorValue(chan);
        return sensor_value_to_float(&val);
    }

    double GetSensorValueDouble(sensor_channel chan) {
        sensor_value val = GetSensorValue(chan);
        return sensor_value_to_double(&val);
    }

    bool IsReady() {
        return isInitialized;
    }

protected:
    const device &dev;

    ~CSensorDevice() = default;

private:
    bool isInitialized;
};

#endif //C_SENSOR_DEVICE_H
