#include "common.h"

#include <cmath>
#include <zephyr/drivers/sensor.h>
#include <zephyr/fs/fs.h>

int read_ina(const struct device *ina_dev, float &voltage, float &current) {

    struct sensor_value value;
    int ret = sensor_sample_fetch(ina_dev);
    if (ret != 0) {
        return ret;
    }
    sensor_channel_get(ina_dev, SENSOR_CHAN_CURRENT, &value);
    current = sensor_value_to_float(&value);
    if (current > 78) {
        // man the EEs gotta be missing something
        current = 0;
    }

    sensor_channel_get(ina_dev, SENSOR_CHAN_VOLTAGE, &value);
    voltage = sensor_value_to_float(&value);
    return 0;
}

int read_barom(const struct device *barom_dev, float &temp, float &press) {
    int ret = sensor_sample_fetch(barom_dev);
    if (ret != 0) {
        return ret;
    }
    struct sensor_value val;
    sensor_channel_get(barom_dev, SENSOR_CHAN_PRESS, &val);
    press = sensor_value_to_float(&val);

    sensor_channel_get(barom_dev, SENSOR_CHAN_AMBIENT_TEMP, &val);
    temp = sensor_value_to_float(&val);
    return 0;
}

int read_imu_up(const struct device *imu_dev, float &vert_axis) {
    int ret = sensor_sample_fetch(imu_dev);
    if (ret != 0) {
        return ret;
    }
    struct sensor_value val;
    ret = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_X, &val);
    if (ret != 0) {
        return ret;
    }
    vert_axis = sensor_value_to_float(&val);
    return 0;
}

int set_lsm_sampling(const struct device *imu_dev, int odr) {
#ifdef CONFIG_BOARD_NATIVE_SIM
    return 0;
#endif
    struct sensor_value sampling = {0};
    sensor_value_from_float(&sampling, odr);
    int ret = sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling);
    if (ret < 0) {
        printk("LSM6DSL: Couldnt set accel sampling\n");
        return ret;
    }
    ret = sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling);
    if (ret < 0) {
        printk("LSM6DSL: Couldnt set gyro sampling\n");
        return ret;
    }
    return 0;
}

// Needed for sys init (seemingly)
extern "C" {
int init_boostdata_locked() {
    return 0;
}
}
SYS_INIT(init_boostdata_locked, APPLICATION, 0);
