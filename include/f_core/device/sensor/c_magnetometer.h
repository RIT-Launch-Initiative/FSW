#ifndef C_MAGNETOMETER_H
#define C_MAGNETOMETER_H

#ifndef CONFIG_F_CORE_SENSOR
#error "In order to use these APIs, set CONFIG_F_CORE_SENSOR=y"
#endif

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
     * See parent docs
     */
    bool UpdateSensorValue() override;

    /**
     * See parent docs
     */
    sensor_value GetSensorValue(sensor_channel chan) const override;

  protected:
    /**
     * Destructor
     */
    ~CMagnetometer() = default;

  private:
    using CBase = CSensorDevice;

    typedef struct {
        sensor_value x;
        sensor_value y;
        sensor_value z;
    } SMagnetometerData;

    SMagnetometerData magData;
};

#endif //C_MAGNETOMETER_H
