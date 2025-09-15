#include "udp_transmit.h"

#include "zephyr/zbus/zbus.h"

#include <string>
#include <f_core/utils/c_hashmap.h>
#include <f_core/net/transport/c_udp_socket.h>

void transmitRawData(const zbus_channel* chan) {
    CHashMap<std::string, void*> userData = *static_cast<CHashMap<std::string, void*>*>(zbus_chan_user_data(chan));
    CUdpSocket* socket = static_cast<CUdpSocket*>(userData.Get("TelemetrySocket").value_or(nullptr));
    if (socket == nullptr) {
        LOG_ERR("No socket was set for telemetry transmission");
        return;
    }

    socket->TransmitAsynchronous(zbus_chan_const_msg(chan), zbus_chan_msg_size(chan));
}

void transmitDownlinkData(const zbus_channel* chan) {
    CHashMap<std::string, void*> userData = *static_cast<CHashMap<std::string, void*>*>(zbus_chan_user_data(chan));
    CUdpSocket* socket = static_cast<CUdpSocket*>(userData.Get("DownlinkSocket").value_or(nullptr));
    if (socket == nullptr) {
        LOG_ERR("No socket was set for LoRa transmission");
        return;
    }
    socket->TransmitAsynchronous(zbus_chan_const_msg(chan), zbus_chan_msg_size(chan));
}
