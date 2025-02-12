#include "c_lora_transmit_tenant.h"
#include "c_radio_module.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraTransmitTenant);

void CLoraTransmitTenant::Startup() {
    bool success = portDataMap.Insert(NNetworkDefs::POWER_MODULE_INA_DATA_PORT, {});
    success &= portDataMap.Insert(NNetworkDefs::RADIO_MODULE_GNSS_DATA_PORT, {});
    success &= portDataMap.Insert(NNetworkDefs::SENSOR_MODULE_TELEMETRY_PORT, {});

    if (!success) {
        LOG_ERR("Failed to insert all ports into hashmap");
        k_oops();
    }
}

void CLoraTransmitTenant::PostStartup() {
    // TODO: Blast later
    uint8_t *bufferOne = portDataMap.Get(NNetworkDefs::POWER_MODULE_INA_DATA_PORT).value();
    if (bufferOne == nullptr) {
        LOG_ERR("Failed to get buffer for port %d", NNetworkDefs::POWER_MODULE_INA_DATA_PORT);
        k_oops();
    }


    uint8_t *bufferTwo = portDataMap.Get(NNetworkDefs::RADIO_MODULE_GNSS_DATA_PORT).value();
    if (bufferTwo == nullptr) {
        LOG_ERR("Failed to get buffer for port %d", NNetworkDefs::RADIO_MODULE_GNSS_DATA_PORT);
        k_oops();
    }

    uint8_t *bufferThree = portDataMap.Get(NNetworkDefs::SENSOR_MODULE_TELEMETRY_PORT).value();
    if (bufferThree == nullptr) {
        LOG_ERR("Failed to get buffer for port %d", NNetworkDefs::SENSOR_MODULE_TELEMETRY_PORT);
        k_oops();
    }


    for (uint8_t i = 0; i < 255; i++) {
        bufferOne[i] = i;
        bufferTwo[i] = i;
        bufferThree[i] = i;
    }

    LOG_INF("Buffers populated");
}

void CLoraTransmitTenant::Run() {
    LOG_INF("Entering LoRa transmit tenant");

    NTypes::RadioBroadcastData data{};
    uint8_t txData[256]{};
    if (int ret = loraTransmitPort.Receive(data, K_MSEC(10)); ret < 0) {
        LOG_WRN_ONCE("Failed to receive from message port (%d)", ret);
        return;
    }

    uint8_t *buffer = portDataMap.Get(data.port).value();
    if (buffer != nullptr) {
        memcpy(buffer, data.data, data.size);
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

    memcpy(txData, &data.port, 2); // Copy port number to first 2 bytes
    memcpy(txData + 2, &data.data, data.size); // Copy payload to the rest of the buffer

    LOG_INF("Transmitting %d bytes from port %d over LoRa", data.size, data.port);
    lora.TransmitSynchronous(txData, data.size + 2);
}
