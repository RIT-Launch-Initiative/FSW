#include <zephyr/kernel.h>
#include <app_version.h>
#include <zephyr/device.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/sensor/tmp116.h>

#include <zephyr/logging/log.h>

#include <zephyr/net/socket.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>

#define SLEEP_TIME_MS   1000

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);


int main(void) {
    
    const struct device *dev = DEVICE_DT_GET_ONE(meas_ms5611);

    struct sensor_value pressure;
    struct sensor_value temperature;

    while (true) {
        sensor_sample_fetch(dev);
        sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure);
        sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature);


        printk("Pressure: %f\tTemp: %f\n", sensor_value_to_double(&pressure), sensor_value_to_double(&temperature));

        k_msleep(1000);
    }

	return 0;
}
