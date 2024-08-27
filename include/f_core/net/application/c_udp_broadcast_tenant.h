#ifndef C_UDP_BROADCAST_TENANT_H
#define C_UDP_BROADCAST_TENANT_H

#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/messaging/c_message_port.h>

template <typename T>
class CUdpBroadcastTenant {
public:
    /**
     * Constructor
     * @param ipAddr Source IP address to broadcast from
     * @param srcPort Source port to broadcast from
     * @param dstPort Destination port to broadcast to
     * @param messagePort Message port to receive messages to broadcast
     */
    CUdpBroadcastTenant(const char *ipAddr, const int srcPort, const int dstPort, CMessagePort<T> &messagePort) : udp(CIPv4(ipAddr), srcPort, dstPort), messagesToBroadcast(&messagePort) {}

    /**
     *
     * @param udp UDP socket to broadcast messages to
     * @param messagePort Message port to receive messages to broadcast
     */
    CUdpBroadcastTenant(const CUdpSocket& udp, CMessagePort<T> &messagePort) : udp(udp), messagesToBroadcast(&messagePort) {}

    /**
     * Destructor
     */
    ~CUdpBroadcastTenant() = default;

    /**
     * Synchronously transmit a message received from a message port over UDP
     */
    void TransmitMessageSynchronous() {
        T message{};
        if (messagesToBroadcast->Receive(message, K_FOREVER) == 0) {
            udp.TransmitSynchronous(message, sizeof(T));
        }
    }

    /**
     * Asynchronously transmit a message received from a message port over UDP
     */
    void TransmitMessageAsynchronous() {
        T message{};
        if (messagesToBroadcast->Receive(message, K_NO_WAIT) == 0) {
            udp.TransmitAsynchronous(message, sizeof(T));
        }
    }
private:
    CUdpSocket udp;
    CMessagePort<T> *messagesToBroadcast;
};

#endif //C_UDP_BROADCAST_TENANT_H
