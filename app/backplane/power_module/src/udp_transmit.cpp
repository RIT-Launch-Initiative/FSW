#include "udp_transmit.h"

#include "zephyr/zbus/zbus.h"

#include <string>
#include <f_core/utils/c_hashmap.h>
#include <f_core/net/transport/c_udp_socket.h>

void transmitRawData(const struct zbus_channel* chan) {
    CHashMap<std::string, CUdpSocket> udpSockets = static_cast<CHashMap<std::string, CUdpSocket>>(zbus_chan_user_data(chan));
    udpSockets.Get("TelemetrySocket")->TransmitAsynchronous(zbus_chan_const_msg(chan), zbus_chan_msg_size(chan));
}

void transmitDownlinkData(const struct zbus_channel* chan) {
    CHashMap<std::string, CUdpSocket> udpSockets = static_cast<CHashMap<std::string, CUdpSocket>>(zbus_chan_user_data(chan));
    udpSockets.Get("DownlinkSocket")->TransmitAsynchronous(zbus_chan_const_msg(chan), zbus_chan_msg_size(chan));
}
