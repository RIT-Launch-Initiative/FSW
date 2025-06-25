#ifndef C_UDP_SOCKET_H
#define C_UDP_SOCKET_H

#include "zephyr/posix/poll.h"

#include <cstdint>
#include <f_core/net/c_transciever.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>

class CIPv4;

class CUdpSocket {
public:
    struct SocketServiceUserData {
        CUdpSocket *socket;
        void *userData;
    };

    /**
     * Constructor
     * @param ipv4 IP address instance to bind to
     * @param srcPort Source port to bind to
     * @param dstPort Destination port to send to
     */
    CUdpSocket(const CIPv4& ipv4, uint16_t srcPort, uint16_t dstPort);

    /**
     * Destructor
     */
    ~CUdpSocket();

    /**
     * Transmit data synchronously
     * @param data Data to transmit
     * @param len Length of data to transmit
     * @return Number of bytes transmitted or negative error code
     */
    int TransmitSynchronous(const void* data, size_t len);

    /**
     * Receive data synchronously
     * @param data Buffer to store received data
     * @param len Size of the buffer
     * @param srcAddr Optional source address
     * @param srcAddrLen Optional source address length
     * @return Number of bytes received or negative error code
     */
    int ReceiveSynchronous(void* data, size_t len, sockaddr* srcAddr = nullptr, socklen_t* srcAddrLen = nullptr);

    /**
     * Transmit data asynchronously
     * @param data Data to transmit
     * @param len Length of data to transmit
     * @return Number of bytes transmitted or negative error code
     */
    int TransmitAsynchronous(const void* data, size_t len);

    /**
     * Receive data asynchronously
     * @param data Buffer to store received data
     * @param len Size of the buffer
     * @param srcAddr Optional source address
     * @param srcAddrLen Optional source address length
     * @return Number of bytes received or negative error code
     */
    int TransmitAsynchronous(const void* data, size_t len, uint16_t dstPort);

    /**
     * See parent docs
     */
    int ReceiveAsynchronous(void* data, size_t len, sockaddr* srcAddr = nullptr, socklen_t* srcAddrLen = nullptr);

    /**
     * Set transmit timeout
     * @param timeoutMillis Timeout in milliseconds
     * @return 0 on success, negative error code otherwise
     */
    int SetTxTimeout(int timeoutMillis);

    /**
     * Set receive timeout
     * @param timeoutMillis Timeout in milliseconds
     * @return 0 on success, negative error code otherwise
     */
    int SetRxTimeout(int timeoutMillis);

    /**
     * Set destination port
     * @param port Destination port
     */
    void SetDstPort(const int port) {
        dstPort = port;
    }

private:
    // CONFIG_ARCH_POSIX uses loopback for broadcast
#if defined(CONFIG_ARCH_POSIX) && defined(CONFIG_NET_NATIVE_OFFLOADED_SOCKETS)
    static constexpr char BROADCAST_IP[] = "127.0.0.1";
#else
    static constexpr char BROADCAST_IP[] = "255.255.255.255";
#endif

    int sock = -1;
    int dstPort = -1;

    net_socket_service_desc* serviceDesc = nullptr;
    pollfd sockfd = {
        .fd = -1,
        .events = POLLIN,
        .revents = 0
    };
};

#endif //C_UDP_SOCKET_H
