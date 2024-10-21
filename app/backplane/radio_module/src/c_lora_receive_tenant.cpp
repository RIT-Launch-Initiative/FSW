#include "c_lora_receive_tenant.h"
#include "c_radio_module.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraReceiveTenant);

void CLoraReceiveTenant::Startup() {
}

void CLoraReceiveTenant::PostStartup() {
}

void CLoraReceiveTenant::Run() {
    while (true) {
        NRadioModuleTypes::RadioBroadcastData data{};

        // This effectively fills the entire struct.
        // LoRa packet fills into .port and then .data, so subtract 2 bytes from the size
        data.size = lora.ReceiveSynchronous(&data, sizeof(data.data), nullptr, nullptr, K_MSEC(10)) - sizeof(uint16_t);

        udpTransmitPort.Send(data);
    }
}
