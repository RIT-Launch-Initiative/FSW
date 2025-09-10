#include <f_core/device/sensor/c_temperature_sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CTemperatureSensor);

CTemperatureSensor::CTemperatureSensor(const device& dev) : CSensorDevice(dev) {
}

bool CTemperatureSensor::UpdateSensorValue() {
    return CBase::UpdateSensorValue() && (0 == sensor_channel_get(&dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature));
}

sensor_value CTemperatureSensor::GetSensorValue(sensor_channel chan) const {
    if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
        return temperature;
    }

    LOG_ERR("Invalid sensor channel (%d) called for temperature sensor", static_cast<int>(chan));
    k_oops();
    return {INT32_MIN, INT32_MIN};
}