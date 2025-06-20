#include "c_sensor_module.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(flash_listener, CONFIG_LOG_DEFAULT_LEVEL);

ZBUS_CHAN_DECLARE(sensor_data_chan);

static bool flash_listener_active = false;

CDataLogger<NTypes::TimestampedSensorData> dataLogger{"/lfs/sensor_module_data.bin"};

struct flashWorkInfo {
    k_work work;
    NTypes::TimestampedSensorData data;
};

static flashWorkInfo flash_work;

static void flashWorkHandler(k_work* work) {
    flashWorkInfo* info = CONTAINER_OF(work, struct flashWorkInfo, work);
    static int counter = 0;

    dataLogger.Write(info->data);
    counter++;
    if (counter == 5) {
        dataLogger.Sync();
        counter = 0;
    }
}

static void flashCallback(const struct zbus_channel* chan) {
    const NTypes::TimestampedSensorData* data = static_cast<const NTypes::TimestampedSensorData*>(
        zbus_chan_const_msg(chan));

    flash_work.data = *data;
    k_work_submit(&flash_work.work);
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
    // Initialize the work item
    k_work_init(&flash_work.work, flashWorkHandler);

    flash_listener_active = false;
    LOG_INF("Flash listener initialized (initially disabled)");
}
