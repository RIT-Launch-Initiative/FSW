#include "c_udp_listener_tenant.h"
#include "c_radio_module.h"

#include <n_autocoder_types.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CUdpListenerTenant);

void CUdpListenerTenant::Startup() {
    LOG_INF("Listening on port %d", listenPort);
}

void CUdpListenerTenant::PostStartup() {
    udp.SetRxTimeout(10);
}

void CUdpListenerTenant::Run() {
    NTypes::LoRaBroadcastData radioBroadcastData{0};

    // Note len argument is the size of the data buffer, not how much data to receive!
    // rcvResult will contain the actual amount of data received or -1 on error
    const int rcvResult = udp.ReceiveAsynchronous(&radioBroadcastData.Payload, sizeof(radioBroadcastData.Payload));
    if (rcvResult <= 0) {
        return;
    }

    radioBroadcastData.Port = listenPort;
    radioBroadcastData.Size = static_cast<uint8_t>(rcvResult);

    if (loraTransmitPort.Send(radioBroadcastData) == -ENOMSG) {
        LOG_WRN_ONCE("Failed to send to broadcast queue");
        loraTransmitPort.Clear();
    }
}
