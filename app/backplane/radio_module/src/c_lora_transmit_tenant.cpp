#include "c_lora_transmit_tenant.h"
#include "c_radio_module.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraTransmitTenant);

void CLoraTransmitTenant::Startup() {
}

void CLoraTransmitTenant::PostStartup() {
}

void CLoraTransmitTenant::Run() {
    NTypes::RadioBroadcastData data{};
    uint8_t txData[256]{};
    if (int ret = loraTransmitPort.Receive(data, K_MSEC(10)); ret < 0) {
        LOG_WRN_ONCE("Failed to receive from message port (%d)", ret);
        return;
    }

    if (data.size > (256 - 2)) {
        // This case should never occur. If it does, then developer is sending too much data
        LOG_ERR("Received data exceeds LoRa packet size from port %d", data.port);
        k_oops();
        return;
    } else if (data.size == 0) {
        // This case should *rarely* occur.
        // TODO: There might be a bug somewhere with copying that needs to be investigated.
        LOG_WRN_ONCE("Received data is empty from port %d", data.port);
        // k_oops();
        return;
    }

#ifndef CONFIG_RADIO_MODULE_RECEIVER
    memcpy(txData, &data.port, 2); // Copy port number to first 2 bytes
    memcpy(txData + 2, &data.data, data.size); // Copy payload to the rest of the buffer
#else
    memcpy(txData, &data, sizeof(data.data));
#endif
    LOG_INF("Transmitting %d bytes from port %d over LoRa", data.size, data.port);
    lora.TransmitSynchronous(txData, data.size);
}
