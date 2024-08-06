#ifndef C_ACCELEROMETER_DEVICE_H
#define C_ACCELEROMETER_DEVICE_H

#ifndef CONFIG_F_CORE_SENSOR
#error "In order to use these APIs, set CONFIG_F_CORE_SENSOR=y"
#endif

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
    bool UpdateSensorValue();

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
    } SAccelerometerData;

    SAccelerometerData acceleration;
};

#endif //C_ACCELEROMETER_DEVICE_H
