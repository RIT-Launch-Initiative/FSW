#include "f_core/net/application/c_tftp_server_tenant.h"

#include "f_core/os/c_file.h"

#include <cstdio>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CTftpServerTenant);

char *inet_ntoa(struct in_addr in)
{
    static char buf[INET_ADDRSTRLEN];
    unsigned char *bytes = (unsigned char *)&in.s_addr;

    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);

    return buf;
}


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

    if (strncmp(filename, "tree", 4) == 0) {
        if (const int ret = generateTree(); ret < 0) {
            LOG_ERR("Error generating tree");
            return;
        }
    }

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

    CFile file(filename, FS_O_READ);
    if (file.GetStatus() < 0) {
        LOG_ERR("Error opening file %s", filename);
        return;
    }

    const off_t fileSize = static_cast<off_t>(file.GetFileSize());
    if (fileSize == 0) {
        LOG_ERR("Error getting file size for %s", filename);
        return;
    }


    const uint8_t dataBlockOffset = 4;
    const uint16_t opcode = DATA;
    uint16_t blockNumber = 1;

    for (off_t offset = 0; offset < fileSize; offset += 512) {
        uint8_t response[516] = {0}; // 2 byte opcode. 2 byte block number. 512 byte data
        uint8_t *dataBlock = &response[dataBlockOffset];

        // Fill in with DATA opcode
        response[0] = opcode >> 8;
        response[1] = opcode & 0xFF;

        // Fill in with block number
        response[2] = blockNumber >> 8;
        response[3] = blockNumber & 0xFF;

        // Fill in with data
        const size_t readLen = file.Read(dataBlock, 512, offset);

        if (readLen == 0) {
            LOG_ERR("Error reading file %s", filename);
            return;
        }

        sock.TransmitAsynchronous(response, readLen + dataBlockOffset);

        blockNumber++;
    }
}

int CTftpServerTenant::generateTree() {
    // TODO: Implement in another PR
    CFile file("tree", FS_O_WRITE | FS_O_CREATE);
    file.Write("reee", 4);

    return -1;
}
