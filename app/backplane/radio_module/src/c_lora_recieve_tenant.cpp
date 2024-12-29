#include "c_lora_to_udp_tenant.h"
#include "c_radio_module.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraReceiveTenant);

void CLoraRecieveTenant::Startup() {
}

void CLoraRecieveTenant::PostStartup() {
}

void CLoraRecieveTenant::Run() {
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

    LOG_INF("Received %d bytes on port %d", size, buffer[1] << 8 | buffer[0]);
    if (size > 2) {

        if ((buffer[1] << 8 | buffer[0]) == 12000) { // Command
            // Apply commands to pins
            for (int i = 2; i < 6 i++) {
                gpios[i-2].pin_set(buffer[i]);
            }

            // Get status of pins
            for (int i = 2; i < 6; i++) {
                buffer[i] = gpios[i-2].pin_get();
            }

            // Retransmit status so GS can verify
            udp.SetDstPort(12001);
            udp.TransmitAsynchronous(&buffer[2], size);
        } else {
            udp.SetDstPort(buffer[1] << 8 | buffer[0]);
            udp.TransmitAsynchronous(&buffer[2], size);
        }
    }
}
