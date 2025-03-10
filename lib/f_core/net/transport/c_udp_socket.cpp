#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/net/network/c_ipv4.h>

#include <zephyr/net/socket.h>

#include <zephyr/logging/log.h>
#include <zephyr/posix/fcntl.h>

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

#if !defined(CONFIG_ARCH_POSIX) && !defined(CONFIG_NET_NATIVE_OFFLOADED_SOCKETS)
    sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(srcPort),
        .sin_addr = INADDR_ANY // Bind to all interfaces TODO: Might not need ipv4 variable anymore
    };

    if (zsock_bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERR("Failed to bind socket.");
        zsock_close(sock);
    }
#else
    LOG_WRN("Skipping bind. Using native_sim loopback");
#endif

    // Link takes around 2 seconds to come up. Wait to avoid errors when tx/rxing
    const uint32_t uptime = k_uptime_get_32();
    if (uptime < 2000) {
        k_msleep(2000 - uptime);
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

    z_impl_net_addr_pton(AF_INET, BROADCAST_IP, const_cast<in_addr*>(&addr.sin_addr));

    int ret = zsock_sendto(sock, data, len, 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (ret < 0) {
        LOG_ERR("Failed to send broadcast message (%d)", ret);
    }

    return ret;
}

int CUdpSocket::ReceiveSynchronous(void* data, size_t len, sockaddr *srcAddr, socklen_t *srcAddrLen) {
    return zsock_recvfrom(sock, data, len, 0, srcAddr, srcAddrLen);
}

int CUdpSocket::TransmitAsynchronous(const void* data, size_t len) {
    static const sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(dstPort),
    };
    int flags = zsock_fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        LOG_ERR("Failed to get socket flags (%d)", flags);
        return -1;
    }

    if (!(flags & O_NONBLOCK)) {
        flags |= O_NONBLOCK;
        if (zsock_fcntl(sock, F_SETFL, flags) < 0) {
            LOG_ERR("Failed to set socket to non-blocking mode.");
            return -1;
        }
    }

    z_impl_net_addr_pton(AF_INET, "10.0.0.0", const_cast<in_addr*>(&addr.sin_addr));

    int ret = zsock_sendto(sock, data, len, 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (ret < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
        LOG_ERR("Failed to send async message (%d)", errno);
    }

    return ret;
}

int CUdpSocket::ReceiveAsynchronous(void* data, size_t len, sockaddr *srcAddr, socklen_t *srcAddrLen) {
    int flags = zsock_fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        LOG_ERR("Failed to get socket flags (%d)", flags);
        return -1;
    }

    if (!(flags & O_NONBLOCK)) {
        flags |= O_NONBLOCK;
        if (zsock_fcntl(sock, F_SETFL, flags) < 0) {
            LOG_ERR("Failed to set socket to non-blocking mode.");
            return -1;
        }
    }

    const int ret = zsock_recvfrom(sock, data, len, 0, srcAddr, srcAddrLen);
    if (ret < 0) {
        if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
            return 0;
        }
        LOG_ERR("Failed to receive data asynchronously (%d)", errno);
        return -1;
    }

    return ret;
}

int CUdpSocket::SetTxTimeout(const int timeoutMillis) {
    return zsock_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeoutMillis, sizeof(timeoutMillis));
}

int CUdpSocket::SetRxTimeout(const int timeoutMillis) {
    return zsock_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeoutMillis, sizeof(timeoutMillis));
}
