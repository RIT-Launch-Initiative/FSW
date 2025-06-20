#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include "messages.h"

LOG_MODULE_REGISTER(ethernet_listener, CONFIG_LOG_DEFAULT_LEVEL);

ZBUS_CHAN_DECLARE(sensor_data_chan);

static void sendEth(const struct sensor_data_msg* data) {
    LOG_INF("Sending over Ethernet - x: %d, y: %d, z: %d, timestamp: %u",
            data->x, data->y, data->z, data->timestamp);
}

static void ethCallback(const struct zbus_channel* chan) {
    const sensor_data_msg* data = zbus_chan_const_msg(chan);

    sendEth(data);
}

ZBUS_LISTENER_DEFINE(ethernet_lis, ethCallback);

void ethListenerInit(void) {
    zbus_chan_add_obs(&sensor_data_chan, &ethernet_lis, K_MSEC(100));

    LOG_INF("Ethernet listener initialized");
}
