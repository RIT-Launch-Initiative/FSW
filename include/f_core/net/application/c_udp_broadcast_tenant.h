#ifndef C_UDP_BROADCAST_TENANT_H
#define C_UDP_BROADCAST_TENANT_H

#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/messaging/c_message_port.h>

template <typename T>
class CUdpBroadcastTenant {
public:
    CUdpBroadcastTenant(const char *ipAddr, const int srcPort, const int dstPort, CMessagePort<T> &messagePort) : ip(CIPv4(ipAddr)), udp(ip, srcPort, dstPort), messagesToBroadcast(&messagePort) {}

    ~CUdpBroadcastTenant() = default;

    /**
     * Synchronously transmit a message received from a message port over UDP
     */
    void TransmitQueuedSync() {
        T message{};
        if (messagesToBroadcast->Receive(message, K_FOREVER) == 0) {
            udp.TransmitSynchronous(message, sizeof(T));
        }
    }

    /**
     * Asynchronously transmit a message received from a message port over UDP
     */
    void TransmitQueuedAsync() {
        T message{};
        if (messagesToBroadcast->Receive(message, K_NO_WAIT) == 0) {
            udp.TransmitAsynchronous(message, sizeof(T));
        }
    }
private:
    CIPv4 ip;
    CUdpSocket udp;
    CMessagePort<T> *messagesToBroadcast;
};

#endif //C_UDP_BROADCAST_TENANT_H
