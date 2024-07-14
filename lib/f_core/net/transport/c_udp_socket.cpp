#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <f_core/net/transport/c_udp_socket.h>
// #include <f_core/net/network/c_ipv4.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_ARCH_POSIX)
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#include <zephyr/net/socket.h>
#include <zephyr/posix/arpa/inet.h>
#endif
LOG_MODULE_REGISTER(CUdpSocket);

CUdpSocket::CUdpSocket(CIPv4& ip, uint16_t srcPort, uint16_t dstPort) {
    in_addr subnet{};
    // if (ip.Initialize()) {
    //     // Guarantee IPv4 is initialized
    //     return;
    // }

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        return;
    }

    sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(srcPort),
        // .sin_addr = ip.GetAddr()
    };

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERR("Failed to bind socket.");
        close(sock);
    }
}

int CUdpSocket::TransmitSynchronous(const void* data, size_t len) {
    static const sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(dstPort),
        .sin_addr = inet_addr("255.255.255.255")
    };

    int ret = sendto(sock, data, len, 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (ret < 0) {
        LOG_ERR("Failed to send broadcast message (%d)", ret);
    }

    return ret;
}

int CUdpSocket::ReceiveSynchronous(void* data, size_t len) {
    return recvfrom(sock, data, len, 0, nullptr, nullptr);
}


int CUdpSocket::TransmitAsynchronous(const void* data, size_t len) {
    static const sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(dstPort),
        .sin_addr = inet_addr("255.255.255.255")
    };

    int ret = sendto(sock, data, len, 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (ret < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
        LOG_ERR("Failed to send async message (%d)", ret);
    }

    return ret;
}

int CUdpSocket::ReceiveAsynchronous(void* data, size_t len) {
    static pollfd fds{
        .fd = sock,
        .events = POLLIN
    };

    int ret = poll(&fds, 1, 0);
    if (ret < 0) {
        LOG_ERR("Polling error (%d)", ret);
        return ret;
    }

    return recvfrom(sock, data, len, 0, nullptr, nullptr);
}