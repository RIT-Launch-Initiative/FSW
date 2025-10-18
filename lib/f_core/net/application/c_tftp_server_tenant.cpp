#include "f_core/net/application/c_tftp_server_tenant.h"
#include "f_core/os/c_file.h"

#include <cstdio>
#include <cstring>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket_service.h>

LOG_MODULE_REGISTER(CTftpServerTenant);

char *inet_ntoa(in_addr in) {
    static char buf[INET_ADDRSTRLEN];
    unsigned char *bytes = (unsigned char *)&in.s_addr;

    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);

    return buf;
}

extern net_socket_service_desc tftpSocketService;
extern "C" void tftpSocketServiceHandler(net_socket_service_event* pev) {
    auto userData = static_cast<CUdpSocket::SocketServiceUserData*>(pev->user_data);

    if (userData == nullptr) {
        LOG_ERR("User data is null in alertSocketServiceHandler");
        k_oops();
    }

    CCallbackTenant* tenant = static_cast<CCallbackTenant*>(userData->userData);
    if (tenant == nullptr) {
        LOG_ERR("Tenant is null in tftpSocketServiceHandler");
        k_oops();
    }

    tenant->Callback();
}

void CTftpServerTenant::Register() {
    int ret = sock.RegisterSocketService(&tftpSocketService, this);
    if (ret < 0) {
        LOG_ERR("Failed to register socket service for CTftpServerTenant: %d", ret);
    }
}

void CTftpServerTenant::Cleanup() {
}

void CTftpServerTenant::Callback() {
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

    // Opcode is the first two bytes in network order.
    uint16_t opcode = (packet[0] << 8) | packet[1];
    switch (opcode) {
        case RRQ:
            handleReadRequest(srcAddr, packet, rxLen);
            break;
        default:
            LOG_ERR("Undefined opcode operation %d", opcode);
    }
}

int CTftpServerTenant::waitForAck(CUdpSocket &dataSock, const sockaddr &srcAddr, uint16_t blockNum) {
    static constexpr uint16_t ackPktSize = 4;
    uint8_t ack[ackPktSize] = {0}; // 2 bytes for opcode, 2 for block number
    const int maxRetries = 5;
    int retries = 0;

    while (retries < maxRetries) {
        int ret = dataSock.ReceiveAsynchronous(ack, 4, const_cast<sockaddr *>(&srcAddr), nullptr);
        if (ret < 0) {
            // If no data available yet, wait and retry.
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                k_sleep(K_MSEC(100));
                retries++;
                continue;
            } else {
                LOG_ERR("Failed to receive ACK (%d)", errno);
                return errno;
            }
        } else if (ret == ackPktSize) {
            uint16_t receivedOpcode = (ack[0] << 8) | ack[1];
            uint16_t receivedBlockNum = (ack[2] << 8) | ack[3];

            if (receivedOpcode == ACK && receivedBlockNum == blockNum) {
                return 0; // Valid ACK received.
            }

            LOG_ERR("Received invalid ACK. Expected block %d, received block %d", blockNum, receivedBlockNum);
            return -1;
        }
    }

    LOG_ERR("Timeout waiting for ACK for block %d", blockNum);
    return -1;
}

void CTftpServerTenant::handleReadRequest(const sockaddr &clientAddr, const uint8_t *packet, int len) {
    static constexpr uint16_t maxFilenameLen = rwRequestPacketSize - strlen(tftpModeStrings[NETASCII]) + 4; // 4 for opcode and both zero terminators
    const char *filename = reinterpret_cast<const char *>(&packet[2]);
    const uint16_t filenameLen = strnlen(filename, maxFilenameLen);
    const char *modeStr = reinterpret_cast<const char *>(&packet[2 + filenameLen + 1]);
    TftpMode mode = UNDEFINED_TFTP_MODE;

    // Handle a special "tree" request.
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

    LOG_INF("Received read request for %s from %s", filename,
            inet_ntoa(reinterpret_cast<const sockaddr_in *>(&clientAddr)->sin_addr));

    CFile file(filename, FS_O_READ);
    if (file.GetInitStatus() < 0) {
        LOG_ERR("Error opening file %s", filename);
        return;
    }

    const off_t fileSize = static_cast<off_t>(file.GetFileSize());
    if (fileSize < 0) {
        LOG_ERR("Error getting file size for %s", filename);
        return;
    }

    uint16_t clientPort = reinterpret_cast<const sockaddr_in *>(&clientAddr)->sin_port;
    clientPort = (clientPort >> 8) | (clientPort << 8);
    
    CUdpSocket dataSock(ip, 0, clientPort);

    uint16_t blockNumber = 1;
    constexpr uint16_t opcode = DATA;
    constexpr int maxDataSize = 512;
    off_t offset = 0;

    while (offset < fileSize) {
        uint8_t response[516] = {0}; // 2 bytes opcode, 2 bytes block number, up to 512 bytes data
        size_t readLen = file.Read(&response[4], maxDataSize, offset);
        if (readLen <= 0) {
            LOG_ERR("Error reading file %s", filename);
            return;
        }

        // Fill in with DATA opcode
        response[0] = opcode >> 8;
        response[1] = opcode & 0xFF;
        response[2] = blockNumber >> 8;
        response[3] = blockNumber & 0xFF;

        // Transmit the data block on the data socket.
        LOG_INF("Sending block %d of size %d", blockNumber, readLen);
        dataSock.TransmitAsynchronous(response, readLen + 4);

        // Wait for ACK before sending the next block.
        LOG_INF("Waiting for ACK for block %d", blockNumber);
        if (waitForAck(dataSock, clientAddr, blockNumber) != 0) {
            LOG_ERR("Failed to receive ACK for block %d", blockNumber);
            return;
        }
        LOG_INF("Received ACK for block %d", blockNumber);
        blockNumber++;
        offset += readLen;
    }

    // If the file size is an exact multiple of 512, send a final empty data packet.
    if ((fileSize % maxDataSize) == 0) {
        uint8_t finalResponse[4] = {0};
        finalResponse[0] = opcode >> 8;
        finalResponse[1] = opcode & 0xFF;
        finalResponse[2] = blockNumber >> 8;
        finalResponse[3] = blockNumber & 0xFF;
        dataSock.TransmitAsynchronous(finalResponse, 4);
        waitForAck(dataSock, clientAddr, blockNumber);
    }
}

int CTftpServerTenant::generateTree() {
    CFile file("/lfs/tree", FS_O_WRITE | FS_O_CREATE);
    file.Write("reee", 4);
    return -1;
}
