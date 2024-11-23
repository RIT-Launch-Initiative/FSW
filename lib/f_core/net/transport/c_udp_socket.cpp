#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/net/network/c_ipv4.h>

#include <zephyr/net/socket.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CUdpSocket);

CUdpSocket::CUdpSocket(const CIPv4& ipv4, uint16_t srcPort, uint16_t dstPort) : dstPort(dstPort) {
    if (!ipv4.IsInitialized()) {
        // Guarantee IPv4 is initialized
        LOG_ERR("Failed to initialize IPv4 address %s", ipv4.GetIp());
        return;
    }

    sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        return;
    }

    sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(srcPort),
        .sin_addr = INADDR_ANY // Bind to all interfaces TODO: Might not need ipv4 variable anymore
    };

    if (zsock_bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERR("Failed to bind socket.");
        zsock_close(sock);
    }
}

CUdpSocket::~CUdpSocket() {
    zsock_close(sock);
}

int CUdpSocket::TransmitSynchronous(const void* data, size_t len) {
    static const sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(dstPort),
    };

    z_impl_net_addr_pton(AF_INET, "255.255.255.255", const_cast<in_addr*>(&addr.sin_addr));

    int ret = zsock_sendto(sock, data, len, 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (ret < 0) {
        LOG_ERR("Failed to send broadcast message (%d)", ret);
    }

    return ret;
}

int CUdpSocket::ReceiveSynchronous(void* data, size_t len) {
    return zsock_recvfrom(sock, data, len, 0, nullptr, nullptr);
}

int CUdpSocket::TransmitAsynchronous(const void* data, size_t len) {
    static const sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(dstPort),
    };
    z_impl_net_addr_pton(AF_INET, "255.255.255.255", const_cast<in_addr*>(&addr.sin_addr));

    int ret = zsock_sendto(sock, data, len, 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (ret < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
        LOG_ERR("Failed to send async message (%d)", ret);
    }

    return ret;
}

int CUdpSocket::ReceiveAsynchronous(void* data, size_t len) {
    static zsock_pollfd fds{
        .fd = sock,
        .events = ZSOCK_POLLIN
    };

    int ret = zsock_poll(&fds, 1, 0);
    if (ret < 0) {
        LOG_ERR("Polling error (%d)", ret);
        return ret;
    }

    return zsock_recvfrom(sock, data, len, 0, nullptr, nullptr);
}

int CUdpSocket::SetTxTimeout(const int timeoutMillis) {
    return zsock_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeoutMillis, sizeof(timeoutMillis));
}

int CUdpSocket::SetRxTimeout(const int timeoutMillis) {
    return zsock_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeoutMillis, sizeof(timeoutMillis));
}
