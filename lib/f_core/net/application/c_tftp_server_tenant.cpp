#include "f_core/net/application/c_tftp_server_tenant.h"

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CTftpServerTenant);

void CTftpServerTenant::Startup() {
}

void CTftpServerTenant::Cleanup() {
}

void CTftpServerTenant::Run() {
    // TODO: Wait for a read request
    uint8_t packet[rwRequestPacketSize] = {0};
    uint8_t rxLen = sock.ReceiveAsynchronous(packet, rwRequestPacketSize);
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
            break;

        default:
            // We currently don't care about other operations
            LOG_ERR("Undefined opcode operation %d", opcode);
    }


}

void CTftpServerTenant::handleReadRequest(const char *filename, const char *mode, const CIPv4& clientAddr, uint16_t clientPort) {


}