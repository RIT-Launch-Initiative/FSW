#include "f_core/net/application/c_tftp_server_tenant.h"

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CTftpServerTenant);

void CTftpServerTenant::Startup() {
}

void CTftpServerTenant::Cleanup() {
}

void CTftpServerTenant::Run() {
    sockaddr srcAddr = {0};
    socklen_t srcAddrLen = sizeof(srcAddr);
    uint8_t packet[rwRequestPacketSize] = {0};
    uint8_t rxLen = sock.ReceiveAsynchronous(packet, rwRequestPacketSize, &srcAddr, &srcAddrLen);

    if (rxLen == 0) {
        return;
    } else if (rxLen < 0) {
        LOG_ERR("Failed to receive packet (%d)", rxLen);
        return;
    }

    // Opcode is the first two bytes
    uint8_t opcode = packet[0] << 8 | packet[1];
    switch (opcode) {
        case RRQ:
            handleReadRequest(srcAddr, packet, rxLen);
            break;

        default:
            // We currently don't care about other operations
            LOG_ERR("Undefined opcode operation %d", opcode);
    }
}

void CTftpServerTenant::handleReadRequest(const sockaddr& srcAddr, const uint8_t* packet, const uint8_t len) {
    const char *filename = reinterpret_cast<const char*>(&packet[2]); // Skips opcode and expects null terminated string
    const char *modeStr = reinterpret_cast<const char*>(&packet[2 + strlen(filename) + 1]);
    TftpMode mode = UNDEFINED_TFTP_MODE;

    for (int i = 0; i < NUM_MODES; i++) {
        if (strcmp(modeStr, tftpModeStrings[i]) == 0) {
            mode = static_cast<TftpMode>(i);
            break;
        }
    }

    if (mode != OCTET) {
        LOG_ERR("Unsupported TFTP mode %s", modeStr);
        return;
    }

    LOG_INF("Received read request for %s from %s", filename, inet_ntoa(reinterpret_cast<const sockaddr_in*>(&srcAddr)->sin_addr));


}
