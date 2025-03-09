#include <f_core/device/sensor/c_gyroscope.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CGyroscope);

CGyroscope::CGyroscope(const device& dev) : CSensorDevice(dev) {}

bool CGyroscope::UpdateSensorValue() {
    if ((CBase::UpdateSensorValue()) && (0 == sensor_channel_get(&dev, SENSOR_CHAN_GYRO_XYZ, &gyroscopeData.x))) {
        return true;
    }

    return false;
}

sensor_value CGyroscope::GetSensorValue(sensor_channel chan) const {
    switch (chan) {
        case SENSOR_CHAN_GYRO_X:
            return gyroscopeData.x;
        case SENSOR_CHAN_GYRO_Y:
            return gyroscopeData.y;
        case SENSOR_CHAN_GYRO_Z:
            return gyroscopeData.z;
        default:
            // Assert here since this should never occur
            LOG_ERR("Invalid sensor channel (%d) called for gyroscope", chan);
            k_oops();
            return {INT32_MIN, INT32_MIN};
    }
}
