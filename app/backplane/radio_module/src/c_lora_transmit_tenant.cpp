#include "c_lora_transmit_tenant.h"
#include "c_radio_module.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraTransmitTenant);

void CLoraTransmitTenant::Startup() {
}

void CLoraTransmitTenant::PostStartup() {
}

void CLoraTransmitTenant::Run() {
    while (true) {
        NRadioModuleTypes::RadioBroadcastData data{};
        uint8_t txData[256]{};

        if (loraTransmitPort.Receive(data) != 0) {
            LOG_WRN("Failed to receive from message port");
            continue;
        }

        if (data.size > (256 - 2)) {
            LOG_WRN("Received data exceeds LoRa packet size");
            continue;
        }

        memcpy(txData, &data.port, 2);
        memcpy(txData + 2, &data.data, data.size);
        lora.TransmitSynchronous(txData, data.size);
    }
}
