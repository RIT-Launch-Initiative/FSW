#include "f_core/net/application/c_sntp_server_tenant.h"

void CSntpServerTenant::Startup() {}

void CSntpServerTenant::Cleanup() {}

void CSntpServerTenant::Run() {
    sockaddr srcAddr = {0};
    socklen_t srcAddrLen = sizeof(srcAddr);
    uint8_t packet[rwRequestPacketSize] = {0};
    int rxLen = sock.ReceiveAsynchronous(packet, rwRequestPacketSize, &srcAddr, &srcAddrLen);

    if (rxLen == 0) {
        return;
    } else if (rxLen < 0) {
        LOG_ERR("Failed to receive packet (%d)", rxLen);
        return;
    }

    
}
