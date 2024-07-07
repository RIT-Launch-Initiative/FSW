#include <f_core/device/sensor/c_accelerometer.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CAccelerometer);

CAccelerometer::CAccelerometer(const device& dev)
    : CSensorDevice(dev) {
}

bool CAccelerometer::UpdateSensorValue() {
    if ((CBase::UpdateSensorValue()) &&
        (0 == sensor_channel_get(&dev, SENSOR_CHAN_ACCEL_XYZ, &acceleration.x))) {
        return true;
    }

    LOG_WRN("Failed to update sensor values");
    return false;
}

sensor_value CAccelerometer::GetSensorValue(sensor_channel chan) {
    switch (chan) {
        case SENSOR_CHAN_ACCEL_X:
            return acceleration.x;
        case SENSOR_CHAN_ACCEL_Y:
            return acceleration.y;
        case SENSOR_CHAN_ACCEL_Z:
            return acceleration.z;
        default:
            // Assert here since this should never occur
            LOG_ERR("Invalid sensor channel (%d) called for accelerometer", chan);
            k_oops();
            return {INT32_MIN, INT32_MIN};
    }
}
