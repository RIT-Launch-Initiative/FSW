#ifndef C_SENSOR_DEVICE_H
#define C_SENSOR_DEVICE_H

#include <zephyr/device.h>


class CSensorDevice {
public:
    CSensorDevice(const device &device) : device(device), isInitialized(false) {}

    bool Initialize() {
        isInitialized = device_is_ready(&device);
        return isInitialized;
    }

    sensor_value UpdateSensorValue() {
        sensor_sample_fetch(&device);
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

    virtual void *GetSensorValuePtrs();

    bool IsReady() {
        return isInitialized;
    }
private:
    const device &device;
    bool isInitialized;
};



#endif //C_SENSOR_DEVICE_H
