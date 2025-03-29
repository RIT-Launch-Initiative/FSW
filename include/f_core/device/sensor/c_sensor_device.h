#ifndef C_SENSOR_DEVICE_H
#define C_SENSOR_DEVICE_H

#include <zephyr/drivers/sensor.h>

class CSensorDevice {
public:
    /**
     * Constructor
     * @param[in] device Zephyr device structure
     */
    CSensorDevice(const device &device) : dev(device) {
        isInitialized = device_is_ready(&dev);
    }

    /**
     * First part of updating sensor data. Subclasses should call this and then update its own state
     * @return true if fetching the sensor value was successful, false otherwise
     */
    virtual bool UpdateSensorValue() {
        return (isInitialized && 0 == sensor_sample_fetch(&dev));
    }

    /**
     * Get a sensor value from a specific channel
     * @param[in] chan Sensor channel to get the value from
     * @return Sensor value
     */
    virtual sensor_value GetSensorValue(sensor_channel chan) const {
        k_oops();
        return {0, 0};
    };

    /**
     * First part of updating sensor data. Subclasses should call this and then update its own state
     * @return true if fetching the sensor value was successful, false otherwise
     */
    virtual bool ReadSensorValue() {
        return (isInitialized && 0 == sensor_sample_fetch(&dev));
    }

    /**
     * Get a sensor value from a specific channel
     * @param[in] chan Sensor channel to get the value from
     * @return Sensor value
     */
    virtual sensor_value DecodeSensorValue(sensor_channel chan) const {
        k_oops();
        return {0, 0};
    };

    /**
     * Get a sensor value from a specific channel as a float
     * @param[in] chan Sensor channel to get the value from
     * @return Sensor value as a float
     */
    float GetSensorValueFloat(sensor_channel chan) const {
        sensor_value val = GetSensorValue(chan);
        return sensor_value_to_float(&val);
    }

    /**
     * Get a sensor value from a specific channel as a double
     * @param[in] chan Sensor channel to get the value from
     * @return Sensor value as a double
     */
    double GetSensorValueDouble(sensor_channel chan) const {
        sensor_value val = GetSensorValue(chan);
        return sensor_value_to_double(&val);
    }

    /**
     * Configure the sensor device's attribute
     * @param channel Sensor channel to set the attribute for
     * @param attr Attribute to set
     * @param val Value to set the attribute
     * @return Zephyr status code
     */
    int Configure(const sensor_channel channel, const sensor_attribute attr, const sensor_value *val) const {
        return sensor_attr_set(&dev, channel, attr, val);
    }

    /**
     * Get whether the device is ready or not
     * @return true if the device is ready, false otherwise
     */
    bool IsReady() const {
        return isInitialized;
    }

    /**
     * Get the name of the device
     * @return name of the device
     */
    const char *GetName() const {
        return dev.name;
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
