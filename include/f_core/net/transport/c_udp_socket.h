#ifndef C_UDP_SOCKET_H
#define C_UDP_SOCKET_H

#include <cstdint>
#include <f_core/net/c_transciever.h>
#include <zephyr/net/net_ip.h>

class CIPv4;

class CUdpSocket {
public:
    /**
     * Constructor
     * @param ipv4 IP address instance to bind to
     * @param srcPort Source port to bind to
     * @param dstPort Destination port to send to
     */
    CUdpSocket(const CIPv4 &ipv4, uint16_t srcPort, uint16_t dstPort);

    /**
     * Destructor
     */
    ~CUdpSocket();

    /**
     * See parent docs
     */
    int TransmitSynchronous(const void *data, size_t len);

    /**
     * See parent docs
     */
    int ReceiveSynchronous(void *data, size_t len, sockaddr *srcAddr = nullptr, socklen_t *srcAddrLen = nullptr);

    /**
     * See parent docs
     */
    int TransmitAsynchronous(const void *data, size_t len);

    /**
     * See parent docs
     */
    int TransmitAsynchronous(const void *data, size_t len, uint16_t dstPort);

    /**
     * See parent docs
     */
    int ReceiveAsynchronous(void *data, size_t len, sockaddr *srcAddr = nullptr, socklen_t *srcAddrLen = nullptr);

     /**
     * See parent docs
     */
    int SetTxTimeout(int timeoutMillis);

    /**
     * See parent docs
     */
    int SetRxTimeout(int timeoutMillis);

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
};

#endif //C_UDP_SOCKET_H
