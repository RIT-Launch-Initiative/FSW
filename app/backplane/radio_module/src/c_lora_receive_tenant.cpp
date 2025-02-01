#include "c_lora_receive_tenant.h"
#include "c_radio_module.h"

#include <n_autocoder_network_defs.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraReceiveTenant);

void CLoraReceiveTenant::Startup() {
}

void CLoraReceiveTenant::PostStartup() {
}

void CLoraReceiveTenant::Run() {
    uint8_t buffer[255] = {0};
    const int size = lora.ReceiveSynchronous(&buffer, sizeof(buffer), nullptr, nullptr);
    if (size < 0) {
        LOG_ERR("Failed to receive over LoRa (%d)", size);
        return;
    }

    if (size == 0) {
        LOG_WRN("Got 0 bytes from LoRa");
        return;
    }

    const int port = buffer[1] << 8 | buffer[0];
    constexpr int portOffset = 2;
    LOG_DBG("Received %d bytes from LoRa for port %d", size, port);

    if (size > 2) {

        if (port == NNetworkDefs::RADIO_MODULE_COMMAND_PORT) { // Command
            int result;
            // Apply commands to pinsconst
            result = gpios[0].SetPin(buffer[2] & 1);
            LOG_DBG("Set Radiomod pin 0 with return code %d", result);
            result = gpios[1].SetPin((buffer[2] & (1 << 1)) >> 1);
            LOG_DBG("Set Radiomod pin 1 with return code %d", result);
            result = gpios[2].SetPin((buffer[2] & (1 << 2)) >> 2);
            LOG_DBG("Set Radiomod pin 2 with return code %d", result);
            result = gpios[3].SetPin((buffer[2] & (1 << 3)) >> 3);
            LOG_DBG("Set Radiomod pin 3 with return code %d", result);

            // Pack status into RadioBroadcastData
            NTypes::RadioBroadcastData pinStatus = {0};
            pinStatus.port = NNetworkDefs::RADIO_MODULE_COMMAND_RESPONSE_PORT;
            pinStatus.size = size;

            // Get status of pins
            pinStatus.data[0] |= gpios[0].GetPin();
            pinStatus.data[0] |= gpios[1].GetPin() << 1;
            pinStatus.data[0] |= gpios[2].GetPin() << 2;
            pinStatus.data[0] |= gpios[3].GetPin() << 3;

            // Retransmit status so GS can verify
            loraTransmitPort.Send(pinStatus);
        } else {
            udp.SetDstPort(port);
            udp.TransmitAsynchronous(&buffer[2], size - portOffset);
        }
    }
}
