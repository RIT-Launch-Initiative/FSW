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

int read_imu(const struct device *imu_dev, NTypes::AccelerometerData &acc, NTypes::GyroscopeData &gyro) {
    int ret = sensor_sample_fetch(imu_dev);
    if (ret != 0) {
        return ret;
    }
    struct sensor_value vals[3];
    ret = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, vals);
    if (ret != 0) {
        return ret;
    }
    acc.X = sensor_value_to_float(&vals[0]);
    acc.Y = sensor_value_to_float(&vals[1]);
    acc.Z = sensor_value_to_float(&vals[2]);

    ret = sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, vals);
    if (ret != 0) {
        return ret;
    }
    gyro.X = sensor_value_to_float(&vals[0]);
    gyro.Y = sensor_value_to_float(&vals[1]);
    gyro.Z = sensor_value_to_float(&vals[2]);
    return 0;
}

int set_lsm_sampling(const struct device *imu_dev, int odr) {
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

NTypes::AccelerometerData normalize(NTypes::AccelerometerData acc) {
    float magn = sqrtf(acc.X * acc.X + acc.Y * acc.Y + acc.Z * acc.Z);
    return {acc.X / magn, acc.Y / magn, acc.Z / magn};
}

extern bool boostdata_locked;
static struct fs_file_t allowfile;

bool is_data_locked() { return boostdata_locked; }

// Needed for sys init (seemingly)
extern "C" {
int init_boostdata_locked() {
    printk("\nChecking boostdata lock\n");
    fs_file_t_init(&allowfile);
    fs_dirent ent = {};
    int ret = fs_stat(ALLOWFILE_PATH, &ent);
    if (ret != 0) {
        printk("No allowfile found, locked!\n\n");
        boostdata_locked = true;
        return -1;
    } else {
        printk("Allowfile found, flash unlocked\n\n");
        boostdata_locked = false;
        return 0;
    }
}
}
#define DATA_LOCK_PRIORITY 99
static_assert(DATA_LOCK_PRIORITY > CONFIG_FILE_SYSTEM_INIT_PRIORITY, "Relies on FS");
SYS_INIT(init_boostdata_locked, POST_KERNEL, DATA_LOCK_PRIORITY);

small_orientation minify_orientation(const NTypes::AccelerometerData &normed) {
    small_orientation orientation = {0};
    orientation.x = normed.X * 127;
    orientation.y = normed.Y * 127;
    orientation.z = normed.Z * 127;
    return orientation;
}