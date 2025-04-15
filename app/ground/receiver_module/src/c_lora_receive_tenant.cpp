#include "c_lora_receive_tenant.h"
#include "c_receiver_module.h"

#include <n_autocoder_network_defs.h>
#include <n_autocoder_types.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraReceiveTenant);

void CLoraReceiveTenant::Startup() {}

void CLoraReceiveTenant::PostStartup() {}

void CLoraReceiveTenant::Run() {
    int port = 0;
    uint8_t buffer[255] = {0};

    int rxSize = receive(buffer, sizeof(buffer), &port);
    if (rxSize <= 0) {
        return;
    }

    udp.SetDstPort(port);
    udp.TransmitAsynchronous(buffer + portOffset, rxSize - portOffset);
}

int CLoraReceiveTenant::receive(uint8_t* buffer, const int buffSize, int* port) {
    int16_t rssi = 0;
    int8_t snr = 0;

    const int rxSize = loraTransmitTenant.lora.ReceiveSynchronous(buffer, buffSize, &rssi, &snr, K_SECONDS(5));
    if (rxSize == -EAGAIN) {
        return rxSize;
    }

    const NTypes::LoRaReceiveStatistics stats{
        .ReceivedSignalStrengthIndicator = rssi,
        .SignalToNoiseRatio = snr,
    };

    LOG_INF("RSSI: %d SNR: %d", stats.ReceivedSignalStrengthIndicator, stats.SignalToNoiseRatio);
    udp.TransmitAsynchronous(&stats, sizeof(stats), NNetworkDefs::RADIO_MODULE_LORA_RX_STATS_PORT);

    if (rxSize < 0) {
        LOG_ERR("Failed to receive over LoRa (%d)", rxSize);
        return rxSize;
    }

    if (rxSize == 0) {
        LOG_WRN("Got 0 bytes from LoRa");
        return rxSize;
    }

    *port = buffer[1] << 8 | buffer[0];
    LOG_INF("Got data for port %d from LoRa", *port);

    return rxSize;
}
