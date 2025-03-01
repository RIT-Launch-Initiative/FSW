#include <f_core/device/sensor/c_magnetometer.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CMagnetometer);

CMagnetometer::CMagnetometer(const device& dev) : CSensorDevice(dev) {}

bool CMagnetometer::UpdateSensorValue() {
    if ((CBase::UpdateSensorValue()) && (0 == sensor_channel_get(&dev, SENSOR_CHAN_MAGN_XYZ, &magData.x))) {
        return true;
    }

    return false;
}

sensor_value CMagnetometer::GetSensorValue(sensor_channel chan) const {
    switch (chan) {
        case SENSOR_CHAN_MAGN_X:
            return magData.x;
        case SENSOR_CHAN_MAGN_Y:
            return magData.y;
        case SENSOR_CHAN_MAGN_Z:
            return magData.z;
        default:
            // Assert here since this should never occur
            LOG_ERR("Invalid sensor channel (%d) called for magnetometer", chan);
            k_oops();
            return {INT32_MIN, INT32_MIN};
    }
}
