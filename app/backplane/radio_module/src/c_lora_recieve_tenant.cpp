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
            gpios[0].pin_set(buffer[2] & 1);
            gpios[1].pin_set((buffer[2] & (1 << 1)) >> 1);
            gpios[2].pin_set((buffer[2] & (1 << 2)) >> 2);
            gpios[3].pin_set((buffer[2] & (1 << 3)) >> 3);

            // Get status of pins
            buffer[2] &= 0; // clear buffer[2]
            buffer[2] |= gpios[0].pin_get();
            buffer[2] |= gpios[1].pin_get() << 1;
            buffer[2] |= gpios[2].pin_get() << 2;
            buffer[2] |= gpios[3].pin_get() << 3;


            // Retransmit status so GS can verify
            udp.SetDstPort(12001);
            udp.TransmitAsynchronous(&buffer[2], size);
        } else {
            udp.SetDstPort(buffer[1] << 8 | buffer[0]);
            udp.TransmitAsynchronous(&buffer[2], size);
        }
    }
}
