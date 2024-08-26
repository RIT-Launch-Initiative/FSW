#ifndef C_UDP_BROADCAST_TENANT_H
#define C_UDP_BROADCAST_TENANT_H

#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/messaging/c_message_port.h>

template <typename T>
class CUdpBroadcastTenant {
public:
    CUdpBroadcastTenant(const char *ip, const int srcPort, const int dstPort, const CMessagePort<T> &messagePort) : ip(ip), udp(ip, srcPort, dstPort), messagesToBroadcast(messagePort) {}

    ~CUdpBroadcastTenant() = default;

    void TransmitQueuedSync() {
        T message{};
        while (messagesToBroadcast.Receive(message, K_FOREVER) == 0) {
            udp.TransmitSynchronous(message);
        }
    }

    void TransmitQueuedAsync() {
        T message{};
        while (messagesToBroadcast.Receive(message, K_NO_WAIT) == 0) {
            udp.TransmitAsynchronous(message);
        }
    }
private:
    CIPv4 &ip;
    CUdpSocket &udp;
    CMessagePort<T> &messagesToBroadcast;
};



#endif //C_UDP_BROADCAST_TENANT_H
