#include <f_core/device/sensor/c_shunt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CShunt);

CShunt::CShunt(const device& dev)
    : CSensorDevice(dev) {
}

bool CShunt::UpdateSensorValue() {
    sensor_value voltage{};
    sensor_value current{};
    sensor_value power{};

    if (!CBase::UpdateSensorValue()) {
        return false;
    }

    if (sensor_channel_get(&dev, SENSOR_CHAN_VOLTAGE, &voltage) < 0) {
        LOG_ERR("Failed to get pressure from barometer sensor");
        return false;
    }

    if (sensor_channel_get(&dev, SENSOR_CHAN_CURRENT, &current) < 0) {
        LOG_ERR("Failed to get temperature from barometer sensor");
        return false;
    }

    if (sensor_channel_get(&dev, SENSOR_CHAN_POWER, &power) < 0) {
        LOG_ERR("Failed to get temperature from barometer sensor");
        return false;
    }

    shuntData.voltage = voltage;
    shuntData.current = current;
    shuntData.power = power;

    return true;
}

sensor_value CShunt::GetSensorValue(sensor_channel chan) const {
    switch (chan) {
        case SENSOR_CHAN_VOLTAGE:
            return shuntData.voltage;
        case SENSOR_CHAN_CURRENT:
            return shuntData.current;
        case SENSOR_CHAN_POWER:
            return shuntData.power;
        default:
            LOG_ERR("Invalid sensor channel (%d) called for shunt", chan);
            k_oops();
            return sensor_value{INT32_MIN, INT32_MIN};
    }
}