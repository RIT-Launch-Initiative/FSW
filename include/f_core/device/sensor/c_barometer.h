#ifndef C_BAROMETER_DEVICE_H
#define C_BAROMETER_DEVICE_H

#include "c_sensor_device.h"
#include <zephyr/device.h>

class CBarometer : public CSensorDevice {
public:
    /**
     * Constructor
     * @param[in] dev Zephyr device structure
     */
    explicit CBarometer(const device& dev);

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
        sensor_value pressure;
        sensor_value temperature;
    } SBarometerData;

    SBarometerData barometerData;
};



#endif //C_BAROMETER_DEVICE_H
