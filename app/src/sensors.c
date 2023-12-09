#include <zephyr/drivers/sensor.h>
#include <zephyr/zephyr.h>
#include <zephyr/logging/log.h>
#include "sensors.h"


LOG_MODULE_REGISTER(sensors_util, CONFIG_APP_LOG_LEVEL);

static SENSOR_MODULE_DATA_T readings = {0};


static int check_dev(const struct device *device) {
    int ret = device_is_ready(device);

    if (ret) printk("Error %d: %s is not ready\n", ret, device->name);

    return ret;
}


