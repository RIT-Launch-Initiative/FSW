#ifndef C_SENSOR_DEVICE_H
#define C_SENSOR_DEVICE_H

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

class CSensorDevice {
public:
    /**
     * Constructor
     * @param[in] device Zephyr device structure
     */
    CSensorDevice(const device &device) : dev(device), isInitialized(false) {}

    /**
     * Set isInitialized to true if the device is ready
     * @return true if the device is initialized, false otherwise
     */
    bool Initialize() {
        isInitialized = device_is_ready(&dev);
        return isInitialized;
    }

    /**
     * First part of updating sensor data. Subclasses should call this and then update its own state
     * @return true if fetching the sensor value was successful, false otherwise
     */
    virtual bool UpdateSensorValue() {
        return (0 == sensor_sample_fetch(&dev));
    }

    /**
     * Get a sensor value from a specific channel
     * @param[in] chan Sensor channel to get the value from
     * @return Sensor value
     */
    virtual sensor_value GetSensorValue(sensor_channel chan);

    /**
     * Get a sensor value from a specific channel as a float
     * @param[in] chan Sensor channel to get the value from
     * @return Sensor value as a float
     */
    float GetSensorValueFloat(sensor_channel chan) {
        sensor_value val = GetSensorValue(chan);
        return sensor_value_to_float(&val);
    }

    /**
     * Get a sensor value from a specific channel as a double
     * @param[in] chan Sensor channel to get the value from
     * @return Sensor value as a double
     */
    double GetSensorValueDouble(sensor_channel chan) {
        sensor_value val = GetSensorValue(chan);
        return sensor_value_to_double(&val);
    }

    /**
     * Get whether the device is ready or not
     * @return true if the device is ready, false otherwise
     */
    bool IsReady() {
        return isInitialized;
    }

protected:
    const device &dev;

    /**
     * Destructor
     */
    ~CSensorDevice() = default;

private:
    bool isInitialized;
};

#endif //C_SENSOR_DEVICE_H
