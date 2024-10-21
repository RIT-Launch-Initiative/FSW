#include "c_udp_broadcast_tenant.h"

#include <n_radio_module_types.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CBroadcastReceiveTenant);

void CUdpBroadcastTenant::Startup() {
}

void CUdpBroadcastTenant::PostStartup() {
    udp.SetRxTimeout(10);
}

void CUdpBroadcastTenant::Run() {
    while (true) {
        NRadioModuleTypes::RadioBroadcastData radioBroadcastData{0};

        radioBroadcastData.port = listenPort;
        radioBroadcastData.size = udp.ReceiveAsynchronous(&radioBroadcastData.data, sizeof(radioBroadcastData.data));

        loraTransmitPort.Send(radioBroadcastData);
    }
}
