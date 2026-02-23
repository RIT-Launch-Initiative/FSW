#include "n_sensing.hpp"

#include "zephyr/device.h"
#include "zephyr/drivers/sensor.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensor, CONFIG_APP_AIRBRAKE_LOG_LEVEL);

namespace NSensing {

#define IMU_NODE   DT_ALIAS(imu)
#define BAROM_NODE DT_ALIAS(barom)

const device *imu_dev = DEVICE_DT_GET(IMU_NODE);
const device *barom_dev = DEVICE_DT_GET(BAROM_NODE);

int initIMU();
int initBarom();

int InitSensors() {
    int ret = initIMU();
    if (ret < 0) {
        LOG_ERR("Failed to initialize IMU. You're doomed");
        return ret;
    }

    ret = initBarom();
    if (ret < 0) {
        LOG_ERR("Failed to initialize Barometer. You're doomed");
        return ret;
    }
    return 0;
}

int MeasureSensors(float &tempC, float &pressureKPa, float &accelMs2, NTypes::GyroscopeData &gyroDps) {
    // todo, can make this less noisy by kicking off fetch, then doing other stuff, then reading
    int bret = sensor_sample_fetch(barom_dev);
    if (bret < 0) {
        LOG_WRN("Failed to fetch barometer values");
    }
    int iret = sensor_sample_fetch(imu_dev);
    if (iret < 0) {
        LOG_WRN("Failed to fetch IMU readings");
    }

    if (bret == 0) {
        sensor_value press = {0};
        sensor_channel_get(barom_dev, SENSOR_CHAN_PRESS, &press);
        pressureKPa = sensor_value_to_float(&press);
        sensor_value temp = {0};
        sensor_channel_get(barom_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        tempC = sensor_value_to_float(&temp);
    }

    if (iret == 0) {
        sensor_value accZ = {0};
        sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_Z, &accZ);
        accelMs2 = sensor_value_to_float(&accZ);

        sensor_value gyro[3] = {0};
        sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, gyro);
        gyroDps.X = sensor_value_to_float(&gyro[0]);
        gyroDps.Y = sensor_value_to_float(&gyro[1]);
        gyroDps.Z = sensor_value_to_float(&gyro[2]);
    }
    if (bret != 0) {
        return bret;
    }
    return 0;
}

int initIMU() {
    if (!device_is_ready(imu_dev)) {
        LOG_ERR("IMU NOT READY");
        return -1;
    }
    return 0;
}
int initBarom() {
    if (!device_is_ready(barom_dev)) {
        LOG_ERR("IMU NOT READY");
        return -1;
    }

#ifdef CONFIG_MS56XX
    const sensor_value odr{.val1 = 4096, .val2 = 0};
    int ret = sensor_attr_set(barom_dev, SENSOR_CHAN_PRESS, SENSOR_ATTR_OVERSAMPLING, &odr);
    if (ret < 0) {
        LOG_WRN("Barometer pressure oversampling configuration failed. Pressure readings may be inaccurate.");
        return ret;
    }
#endif

    return 0;
}

} // namespace NSensing