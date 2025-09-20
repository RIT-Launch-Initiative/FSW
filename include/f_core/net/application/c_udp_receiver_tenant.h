#ifndef C_UDP_RECEIVER_TENANT_H
#define C_UDP_RECEIVER_TENANT_H

#include "f_core/net/network/c_ipv4.h"
#include "f_core/net/transport/c_udp_socket.h"
#include "f_core/os/c_runnable_tenant.h"
#include "f_core/messaging/c_message_port.h"

template <typename T>
class CUdpReceiveTenant : public CRunnableTenant {
public:
    /**
     * Constructor
     * @param name Name of the tenant
     * @param ipAddr Source IP address to receive from
     * @param srcPort Source port to receive messages over UDP from
     * @param dstPort Destination port to send messages over (not important for receiving)
     * @param messagePort Message port to put messages received over UDP
     */
    CUdpReceiveTenant(const char *name, const char *ipAddr, const int srcPort, const int dstPort, CMessagePort<T> &messagePort) : CRunnableTenant(name), udp(CIPv4(ipAddr), srcPort, dstPort), messagesReceived(&messagePort) {
        udp.SetRxTimeout(0);
    }

    /**
     * Constructor
     * @param name Name of the tenant
     * @param udp UDP socket to receive messages to
     * @param messagePort Message port to put messages received over UDP
     */
    CUdpReceiveTenant(const CUdpSocket& udp, CMessagePort<T> &messagePort) : CRunnableTenant(name), udp(udp), messagesReceived(&messagePort) {
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

        if (udp.ReceiveSynchronous(&message, sizeof(T)) == 0) {
            messagesReceived->Send(message, K_FOREVER)
        }
    }

    /**
     * Asynchronously receive a UDP packet and put it on a message port
     */
    void ReceiveMessageAsynchronous() {
        T message{};

        if (udp.ReceiveAsynchronous(&message, sizeof(T)) == 0) {
            messagesReceived->Send(message, K_NO_WAIT)
        }
    }


    /**
     * See parent docs
     */
    void Startup() override {};

    /**
     * See parent docs
     */
    void PostStartup() override {};

    /**
     * See parent docs
     */
    void Run() override {
        ReceiveMessageAsynchronous();
    }
private:
    CUdpSocket udp;
    CMessagePort<T> *messagesReceived;
};

#endif //C_UDP_RECEIVER_TENANT_H
