#include "c_sensor_module.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(ethernet_listener, CONFIG_LOG_DEFAULT_LEVEL);

ZBUS_CHAN_DECLARE(sensor_data_chan);

static CUdpSocket sock(CIPv4("10.3.2.1"), 13100, 13100);

static void sendEth(const NTypes::TimestampedSensorData* data) {
    sock.TransmitAsynchronous(data, sizeof(NTypes::TimestampedSensorData));
}

static void ethCallback(const zbus_channel* chan) {
    const auto data = static_cast<const NTypes::TimestampedSensorData*>(
        zbus_chan_const_msg(chan));

    sendEth(data);
}

ZBUS_LISTENER_DEFINE(ethernet_lis, ethCallback);

void ethListenerInit() {
    int ret = zbus_chan_add_obs(&sensor_data_chan, &ethernet_lis, K_MSEC(100));
    if (ret != 0) {
        LOG_ERR("Failed to add observer: %d", ret);
        return;
    }
    sock.TransmitSynchronous("Hello, Aaron!", 13);
    LOG_INF("Ethernet listener initialized");
}
