#include "c_lora_receive_tenant.h"
#include "c_receiver_module.h"

#include <n_autocoder_network_defs.h>

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
    udp.TransmitAsynchronous(buffer + 2, rxSize - 2);
}

int CLoraReceiveTenant::receive(uint8_t* buffer, const int buffSize, int* port) const {
    LOG_INF("Waiting for LoRa data");
    const int size = loraTransmitTenant.lora.ReceiveSynchronous(buffer, buffSize, nullptr, nullptr, K_SECONDS(5));
    if (size == -EAGAIN) {
        return size;
    }

    if (size < 0) {
        LOG_ERR("Failed to receive over LoRa (%d)", size);
        return size;
    }

    if (size == 0) {
        LOG_WRN("Got 0 bytes from LoRa");
        return size;
    }

    *port = buffer[1] << 8 | buffer[0];
    LOG_INF("Got data for port %d from LoRa", *port);
    return size;
}
