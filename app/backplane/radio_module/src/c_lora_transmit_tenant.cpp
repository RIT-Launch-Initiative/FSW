#include "c_lora_transmit_tenant.h"
#include "c_radio_module.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraTransmitTenant);

void CLoraTransmitTenant::Startup() {
    bool success = portDataMap.Insert(NNetworkDefs::POWER_MODULE_INA_DATA_PORT, {});
    success &= portDataMap.Insert(NNetworkDefs::RADIO_MODULE_GNSS_DATA_PORT, {});
    success &= portDataMap.Insert(NNetworkDefs::SENSOR_MODULE_TELEMETRY_PORT, {});

    success &= padDataRequestedMap.Insert(NNetworkDefs::POWER_MODULE_INA_DATA_PORT, false);
    success &= padDataRequestedMap.Insert(NNetworkDefs::RADIO_MODULE_GNSS_DATA_PORT, false);
    success &= padDataRequestedMap.Insert(NNetworkDefs::SENSOR_MODULE_TELEMETRY_PORT, false);

    if (!success) {
        LOG_ERR("Failed to insert all ports into hashmap");
        k_oops();
    }
}

void CLoraTransmitTenant::PostStartup() {
    // Nothing to do here
}

void CLoraTransmitTenant::Run() {
    NTypes::RadioBroadcastData data{};
    if (int ret = loraTransmitPort.Receive(data, K_MSEC(10)); ret < 0) {
        LOG_WRN_ONCE("Failed to receive from message port (%d)", ret);
        return;
    }

    uint8_t* buffer = portDataMap.Get(data.port).value().data();
    if (buffer != nullptr) {
        memcpy(buffer, data.data, data.size);
    }
}

void CLoraTransmitTenant::PadRun() {
    NTypes::RadioBroadcastData data{};

    // Iterate on both ports and transmit data if requested
    for (const auto &[port, requested] : padDataRequestedMap) {
        if (requested) {
            data.port = port;
            data.size = portDataMap.Get(port).value().size();
            memcpy(data.data, portDataMap.Get(port).value().data(), data.size);
            transmit(data);
        }

    }
}


void CLoraTransmitTenant::FlightRun() {
    NTypes::RadioBroadcastData data{};
    if (readTransmitQueue(data)) {
        uint8_t* buffer = portDataMap.Get(data.port).value().data();

        if (buffer != nullptr) {
            memcpy(buffer, data.data, data.size);
        }
    }
}


void CLoraTransmitTenant::LandedRun() {
    NTypes::RadioBroadcastData data{};

    if (readTransmitQueue(data) && data.port == NNetworkDefs::RADIO_MODULE_GNSS_DATA_PORT) {
        transmit(data);
    }
}


void CLoraTransmitTenant::GroundRun() {
    NTypes::RadioBroadcastData data{};
    if (readTransmitQueue(data)) {
        transmit(data);
    }
}

void CLoraTransmitTenant::transmit(const NTypes::RadioBroadcastData& data) const {
    std::array<uint8_t, 256> txData{};

    if (data.size > (256 - 2)) {
        // This case should never occur. If it does, then developer is sending too much data
        LOG_ERR("Received data exceeds LoRa packet size from port %d", data.port);
        k_oops();
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

bool CLoraTransmitTenant::readTransmitQueue(NTypes::RadioBroadcastData& data) const {
    if (int ret = loraTransmitPort.Receive(data, K_MSEC(10)); ret < 0) {
        LOG_WRN_ONCE("Failed to receive from message port (%d)", ret);
        return false;
    }

    return true;
}
