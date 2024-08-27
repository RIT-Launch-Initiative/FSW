#ifndef C_UDP_RECEIVER_TENANT_H
#define C_UDP_RECEIVER_TENANT_H

#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/messaging/c_message_port.h>

template <typename T>
class CUdpReceiveTenant {
public:
    /**
     * Constructor
     * @param ipAddr Source IP address to receive from
     * @param srcPort Source port to receive messages over UDP from
     * @param dstPort Destination port to send messages over (not important for receiving)
     * @param messagePort Message port to put messages received over UDP
     */
    CUdpReceiveTenant(const char *ipAddr, const int srcPort, const int dstPort, CMessagePort<T> &messagePort) : udp(CIPv4(ipAddr), srcPort, dstPort), messagesReceived(&messagePort) {
        udp.SetRxTimeout(0);
    }

    /**
     * Constructor
     * @param udp UDP socket to receive messages to
     * @param messagePort Message port to put messages received over UDP
     */
    CUdpReceiveTenant(const CUdpSocket& udp, CMessagePort<T> &messagePort) : udp(udp), messagesReceived(&messagePort) {
        udp.SetRxTimeout(0);
    }

    /**
     * Destructor
     */
    ~CUdpReceiveTenant() = default;

    /**
     * Synchronously receive a UDP packet and put it on a message port
     */
    void ReceiveMessageSynchronous() {
        T message{};

        if (udp.ReceiveSynchronous(message, sizeof(T)) == 0) {
            messagesReceived->Send(message, K_FOREVER)
        }
    }

    /**
     * Asynchronously receive a UDP packet and put it on a message port
     */
    void ReceiveMessageAsynchronous() {
        T message{};

        if (udp.ReceiveAsynchronous(message, sizeof(T)) == 0) {
            messagesReceived->Send(message, K_NO_WAIT)
        }
    }
private:
    CUdpSocket udp;
    CMessagePort<T> *messagesReceived;
};

#endif //C_UDP_RECEIVER_TENANT_H
