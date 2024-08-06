#ifndef C_TEMPERATURE_SENSOR_H
#define C_TEMPERATURE_SENSOR_H

#ifndef CONFIG_F_CORE_SENSOR
#error "In order to use these APIs, set CONFIG_F_CORE_SENSOR=y"
#endif

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
     * See parent docs
     */
    bool UpdateSensorValue() override;

    /**
     * See parent docs
     */
    sensor_value GetSensorValue(sensor_channel chan) const override;

  protected:
    ~CTemperatureSensor() = default;

  private:
    using CBase = CSensorDevice;
    sensor_value temperature{};
};

#endif //C_TEMPERATURE_SENSOR_H
