#include <unistd.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/net/network/c_ipv4.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_ARCH_POSIX)
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include <zephyr/net/socket.h>
#include <zephyr/posix/arpa/inet.h>
#endif

LOG_MODULE_REGISTER(CUdpSocket);

CUdpSocket::CUdpSocket(CIPv4& ip, uint16_t srcPort) {
    in_addr subnet{};
    if (ip.Initialize()) {
        // Guarantee IPv4 is initialized
        return;
    }

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        return;
    }

    sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(srcPort),
        .sin_addr = ip.GetAddr()
    };

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERR("Failed to bind socket.");
        close(sock);
    }
}

int CUdpSocket::TransmitSynchronous(const void* data, size_t len) {

    return 0;
}

int CUdpSocket::ReceiveSynchronous(void* data, size_t len) {
    return 0;
}

int CUdpSocket::TransmitAsynchronous(const void* data, size_t len) {
    return 0;
}

int CUdpSocket::ReceiveAsynchronous(void* data, size_t len) {
    return 0;
}
