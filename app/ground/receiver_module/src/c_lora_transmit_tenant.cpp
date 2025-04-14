#include "c_lora_transmit_tenant.h"
#include "c_receiver_module.h"

#include <array>
#include <n_autocoder_network_defs.h>
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

    if (data.size > (256 - 2)) {
        // This case should never occur. If it does, then developer is sending too much data
        LOG_ERR("Received data exceeds LoRa packet size from port %d", data.port);
        return;
    } else if (data.size == 0) {
        // This case should *rarely* occur.
        LOG_WRN_ONCE("Received data is empty from port %d", data.port);
        return;
    }

    memcpy(txData.begin(), &data.port, 2);             // Copy port number to first 2 bytes
    memcpy(txData.begin() + 2, &data.data, data.size); // Copy payload to the rest of the buffer

    LOG_INF("Transmitting %d bytes from port %d over LoRa", data.size, data.port);
    lora.TransmitSynchronous(txData.data(), data.size + 2);
}

bool CLoraTransmitTenant::readTransmitQueue(NTypes::LoRaBroadcastData& data) const {
    if (int ret = loraTransmitPort.Receive(data, K_MSEC(10)); ret < 0) {
        LOG_WRN_ONCE("Failed to receive from message port (%d)", ret);
        return false;
    }

    return true;
}
