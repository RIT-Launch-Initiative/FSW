#include "f_core/device/sensor/n_sensor_calibrators.h"

#include <zephyr/drivers/sensor.h>


bool NSensorCalibrators::CalibrateADXL375(const device& dev, uint16_t nSamples, GravityOrientation gravityOrientation) {
    if (!device_is_ready(&dev)) {
        return false;
    }

    sensor_value offsetX = {0, 0};
    sensor_value offsetY = {0, 0};
    sensor_value offsetZ = {0, 0};


    return true;
}
