#include "c_sensor_module.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(flash_listener, CONFIG_LOG_DEFAULT_LEVEL);

ZBUS_CHAN_DECLARE(sensor_data_chan);

static bool flash_listener_active = false;

static void writeFlash(const NTypes::TimestampedSensorData* data) {}

static void flashCallback(const struct zbus_channel* chan) {
    const NTypes::TimestampedSensorData* data = static_cast<const NTypes::TimestampedSensorData*>(
        zbus_chan_const_msg(chan));

    writeFlash(data);
}

ZBUS_LISTENER_DEFINE(flash_lis, flashCallback);

void flashLogEnable() {
    if (!flash_listener_active) {
        int ret = zbus_chan_add_obs(&sensor_data_chan, &flash_lis, K_MSEC(100));
        if (ret != 0) {
            LOG_ERR("Failed to add observer: %d", ret);
            return;
        }

        flash_listener_active = true;
        LOG_INF("Flash logging enabled");
    } else {
        LOG_WRN("Flash logging already enabled");
    }
}

void flashLogDisable() {
    if (flash_listener_active) {
        zbus_chan_rm_obs(&sensor_data_chan, &flash_lis, K_MSEC(100));
        flash_listener_active = false;
        LOG_INF("Flash logging disabled");
    } else {
        LOG_WRN("Flash logging already disabled");
    }
}

void flashListenerInit() {
    flash_listener_active = false;
    LOG_INF("Flash listener initialized (initially disabled)");
}
