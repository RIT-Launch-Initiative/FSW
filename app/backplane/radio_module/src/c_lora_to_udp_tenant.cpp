#include "c_lora_to_udp_tenant.h"

#include "c_radio_module.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraReceiveTenant);

void CLoraToUdpTenant::Startup() {}

void CLoraToUdpTenant::PostStartup() {}

void CLoraToUdpTenant::Run() {
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
        udp.SetDstPort(buffer[1] << 8 | buffer[0]);
        udp.TransmitAsynchronous(&buffer[2], size);
    }
}
