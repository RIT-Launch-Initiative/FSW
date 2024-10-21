#include "c_udp_listener_tenant.h"
#include "c_radio_module.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CBroadcastReceiveTenant);

void CUdpListenerTenant::Startup() {
}

void CUdpListenerTenant::PostStartup() {
    udp.SetRxTimeout(10);
}

void CUdpListenerTenant::Run() {
    while (true) {
        NRadioModuleTypes::RadioBroadcastData radioBroadcastData{0};

        radioBroadcastData.port = listenPort;
        radioBroadcastData.size = udp.ReceiveAsynchronous(&radioBroadcastData.data, sizeof(radioBroadcastData.data));

        loraTransmitPort.Send(radioBroadcastData);
    }
}
