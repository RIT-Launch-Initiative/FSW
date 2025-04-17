#include "c_lora_transmit_tenant.h"
#include "c_receiver_module.h"

#include <array>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraTransmitTenant);

void CLoraTransmitTenant::Startup() {
}

void CLoraTransmitTenant::PostStartup() {
    // Nothing to do here
}

void CLoraTransmitTenant::Run() {
    NTypes::LoRaBroadcastData data{};
    if (readTransmitQueue(data)) {
        transmit(data);
    }
}
void CLoraTransmitTenant::transmit(const NTypes::LoRaBroadcastData& data) const {
    std::array<uint8_t, 256> txData{};

    if (data.Size > (256 - 2)) {
        // This case should never occur. If it does, then developer is sending too much data
        LOG_ERR("Received data exceeds LoRa packet size from port %d", data.Port);
        return;
    } else if (data.Size == 0) {
        // This case should *rarely* occur.
        LOG_WRN_ONCE("Received data is empty from port %d", data.Port);
        return;
    }

    memcpy(txData.begin(), &data.Port, 2);             // Copy port number to first 2 bytes
    memcpy(txData.begin() + 2, &data.Payload, data.Size); // Copy payload to the rest of the buffer

    LOG_INF("Transmitting %d bytes from port %d over LoRa", data.Size, data.Port);
    lora.TransmitSynchronous(txData.data(), data.Size + 2);
}

bool CLoraTransmitTenant::readTransmitQueue(NTypes::LoRaBroadcastData& data) const {
    if (int ret = loraTransmitPort.Receive(data, K_MSEC(10)); ret < 0) {
        LOG_WRN_ONCE("Failed to receive from message port (%d)", ret);
        return false;
    }

    return true;
}
