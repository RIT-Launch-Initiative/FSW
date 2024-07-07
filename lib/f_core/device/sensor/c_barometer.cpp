#include <f_core/device/sensor/c_barometer.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CBarometer);

CBarometer::CBarometer(const device& dev)
    : CSensorDevice(dev) {
}

bool CBarometer::UpdateSensorValue() {
    sensor_value pressure{};
    sensor_value temperature{};

    if (!CBase::UpdateSensorValue()) {
        return false;
    }

    if (sensor_channel_get(&dev, SENSOR_CHAN_PRESS, &pressure) < 0) {
        LOG_ERR("Failed to get pressure from barometer sensor");
        return false;
    }

    if (sensor_channel_get(&dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature) < 0) {
        LOG_ERR("Failed to get temperature from barometer sensor");
        return false;
    }

    barometerData.pressure = pressure;
    barometerData.temperature = temperature;

    return true;
}

sensor_value CBarometer::GetSensorValue(sensor_channel chan) {
    switch (chan) {
        case SENSOR_CHAN_PRESS:
            return barometerData.pressure;
        case SENSOR_CHAN_AMBIENT_TEMP:
            return barometerData.temperature;
        default:
            LOG_ERR("Invalid sensor channel (%d) called for barometer", chan);
            k_oops();
            return sensor_value{INT32_MIN, INT32_MIN};
    }
}