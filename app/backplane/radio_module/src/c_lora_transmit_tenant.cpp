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
            // This case should never occur. If it does, then developer is sending too much data
            LOG_ERR("Received data exceeds LoRa packet size");
            k_oops();
            continue;
        }

        memcpy(txData, &data.port, 2); // Copy port numebr to first 2 bytes
        memcpy(txData + 2, &data.data, data.size); // Copy payload to the rest of the buffer
        lora.TransmitSynchronous(txData, data.size);
    }
}
