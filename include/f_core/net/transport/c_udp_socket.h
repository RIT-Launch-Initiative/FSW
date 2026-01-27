#pragma once

#include "zephyr/posix/poll.h"

#include <cstdint>
#include <f_core/net/c_transciever.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>

class CIPv4;

class CUdpSocket {
public:
    struct SocketServiceUserData {
        CUdpSocket* socket;
        void* userData;
    };

    /**
     * Constructor
     * @param[in] ipv4 IP address instance to bind to
     * @param[in] srcPort Source port to bind to
     * @param[in] dstPort Destination port to send to
     */
    CUdpSocket(const CIPv4& ipv4, uint16_t srcPort, uint16_t dstPort);

    /**
     * Destructor
     */
    ~CUdpSocket();

    /**
     * Transmit data synchronously
     * @param[in] data Data to transmit
     * @param[in] len Length of data to transmit
     * @return Number of bytes transmitted or negative error code
     */
    int TransmitSynchronous(const void* data, size_t len);

    /**
     * Receive data synchronously
     * @param[out] data Buffer to store received data
     * @param[in] len Size of the buffer
     * @param[in] srcAddr Optional source address
     * @param[in] srcAddrLen Optional source address length
     * @return Number of bytes received or negative error code
     */
    int ReceiveSynchronous(void* data, size_t len, sockaddr* srcAddr = nullptr, socklen_t* srcAddrLen = nullptr);

    /**
     * Transmit data asynchronously
     * @param[in] data Data to transmit
     * @param[in] len Length of data to transmit
     * @return Number of bytes transmitted or negative error code
     */
    int TransmitAsynchronous(const void* data, size_t len);

    /**
     * Receive data asynchronously
     * @param[out] data Buffer to store received data
     * @param[in] len Size of the buffer
     * @param[in] srcAddr Optional source address
     * @param[in] srcAddrLen Optional source address length
     * @return Number of bytes received or negative error code
     */
    int TransmitAsynchronous(const void* data, size_t len, uint16_t dstPort);

    /**
     * See parent docs
     */
    int ReceiveAsynchronous(void* data, size_t len, sockaddr* srcAddr = nullptr, socklen_t* srcAddrLen = nullptr);

    /**
     * Set transmit timeout
     * @param[in] timeoutMillis Timeout in milliseconds
     * @return 0 on success, negative error code otherwise
     */
    int SetTxTimeout(int timeoutMillis);

    /**
     * Set receive timeout
     * @param[in] timeoutMillis Timeout in milliseconds
     * @return 0 on success, negative error code otherwise
     */
    int SetRxTimeout(int timeoutMillis);

    /**
     * Set destination port
     * @param[in] port Destination port
     */
    void SetDstPort(const int port) {
        dstPort = port;
    }

    /**
     * Register the socket service descriptor
     * @param[in] desc Descriptor for the socket service
     * @param[in] userData User data to pass to the service handler
     */
    int RegisterSocketService(net_socket_service_desc* desc, void* userData);

private:
    // CONFIG_ARCH_POSIX uses loopback for broadcast
#if defined(CONFIG_ARCH_POSIX) && defined(CONFIG_NET_NATIVE_OFFLOADED_SOCKETS)
    static constexpr char BROADCAST_IP[] = "127.0.0.1";
#else
    static constexpr char BROADCAST_IP[] = "255.255.255.255";


    int dstPort = -1;

    net_socket_service_desc* serviceDesc = nullptr;
    zsock_pollfd sockfd = {
        .fd = -1,
        .events = POLLIN,
        .revents = 0
    };
};


