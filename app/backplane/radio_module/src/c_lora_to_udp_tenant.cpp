#include "c_lora_to_udp_tenant.h"
#include "c_radio_module.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CLoraReceiveTenant);

void CLoraToUdpTenant::Startup() {
}

void CLoraToUdpTenant::PostStartup() {
}

void CLoraToUdpTenant::Run() {
    while (true) {
        uint8_t buffer[256] = {0};
        const uint8_t size = lora.ReceiveSynchronous(&buffer, sizeof(buffer), nullptr, nullptr, K_MSEC(10)) - sizeof(uint16_t);

        udp.SetDstPort(buffer[0] << 8 | buffer[1]);
        udp.TransmitAsynchronous(&buffer[2], size);
    }
}
