#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include "messages.h"

LOG_MODULE_REGISTER(flash_listener, CONFIG_LOG_DEFAULT_LEVEL);

ZBUS_CHAN_DECLARE(sensor_data_chan);

static bool flash_listener_active = false;

static void write_to_flash(const struct sensor_data_msg* data) {
    LOG_INF("Writing to flash - x: %d, y: %d, z: %d, timestamp: %u",
            data->x, data->y, data->z, data->timestamp);
}

static void flash_callback(const struct zbus_channel* chan) {
    const sensor_data_msg* data = zbus_chan_const_msg(chan);

    LOG_DBG("Flash callback received data");
    write_to_flash(data);
}

ZBUS_LISTENER_DEFINE(flash_lis, flash_callback);

void flashLogEnable(void) {
    if (!flash_listener_active) {
        /* Add the flash listener to the sensor data channel */
        zbus_chan_add_obs(&sensor_data_chan, &flash_lis, K_MSEC(100));
        flash_listener_active = true;
        LOG_INF("Flash logging enabled");
    } else {
        LOG_WRN("Flash logging already enabled");
    }
}

void flashLogDisable(void) {
    if (flash_listener_active) {
        zbus_chan_rm_obs(&sensor_data_chan, &flash_lis, K_MSEC(100));
        flash_listener_active = false;
        LOG_INF("Flash logging disabled");
    } else {
        LOG_WRN("Flash logging already disabled");
    }
}

void flashListenerInit(void) {
    flash_listener_active = false;

    LOG_INF("Flash listener initialized (initially disabled)");
}
