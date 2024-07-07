#include <f_core/device/sensor/c_temperature_sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CTemperatureSensor);

CTemperatureSensor::CTemperatureSensor(const device& device) : CSensorDevice(device) {
}

bool CTemperatureSensor::UpdateSensorValue() {
    return CBase::UpdateSensorValue() && (0 == sensor_channel_get(&dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature));
}

sensor_value CTemperatureSensor::GetSensorValue(sensor_channel chan) {
    if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
        return temperature;
    }

    LOG_ERR("Invalid sensor channel (%d) called for temperature sensor", chan);
    k_oops();
    return {INT32_MIN, INT32_MIN};
}