#include "c_lora_receive_tenant.h"
#include "c_radio_module.h"

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

        if (port == 12000) { // Command
            // Apply commands to pins
            gpios[0].pin_set(buffer[2] & 1);
            gpios[1].pin_set((buffer[2] & (1 << 1)) >> 1);
            gpios[2].pin_set((buffer[2] & (1 << 2)) >> 2);
            gpios[3].pin_set((buffer[2] & (1 << 3)) >> 3);

            // Pack status into RadioBroadcastData
            NTypes::RadioBroadcastData pinStatus = {0};
            pinStatus.port = 12001;
            pinStatus.size = size;

            // Get status of pins
            pinStatus.data[0] |= gpios[0].pin_get();
            pinStatus.data[0] |= gpios[1].pin_get() << 1;
            pinStatus.data[0] |= gpios[2].pin_get() << 2;
            pinStatus.data[0] |= gpios[3].pin_get() << 3;

            // Retransmit status so GS can verify
            loraTransmitPort.Send(pinStatus);
        } else {
            udp.SetDstPort(port);
            udp.TransmitAsynchronous(&buffer[2], size - portOffset);
        }
    }
}
