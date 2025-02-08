#ifndef C_SHUNT_DEVICE_H
#define C_SHUNT_DEVICE_H

#include "c_sensor_device.h"

class CShunt : public CSensorDevice {
  public:
    /**
     * Constructor
     * @param[in] dev Zephyr Device Structure
     */
    explicit CShunt(const device& dev);

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
        sensor_value voltage;
        sensor_value current;
        sensor_value power;
    } SShuntData;

    SShuntData shuntData;
};

#endif //C_ACCELEROMETER_DEVICE_H
