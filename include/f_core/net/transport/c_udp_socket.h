#ifndef C_UDP_SOCKET_H
#define C_UDP_SOCKET_H

#include <cstdint>
#include <f_core/net/c_transciever.h>

class CIPv4;

class CUdpSocket : public CTransceiver {
public:
    /**
     * Constructor
     * @param ip IP address instance to bind to
     * @param srcPort Source port to bind to
     * @param dstPort Destination port to send to
     */
    CUdpSocket(CIPv4 &ipv4, uint16_t srcPort, uint16_t dstPort);

    /**
     * Destructor
     */
    ~CUdpSocket();

    /**
     * See parent docs
     */
    int TransmitSynchronous(const void *data, size_t len) override;

    /**
     * See parent docs
     */
    int ReceiveSynchronous(void *data, size_t len) override;

    /**
     * See parent docs
     */
    int TransmitAsynchronous(const void *data, size_t len) override;

    /**
     * See parent docs
     */
    int ReceiveAsynchronous(void *data, size_t len) override;

    /**
     * See parent docs
     */
    int SetRxTimeout(int timeout) override;


private:
    int sock = -1;
    int dstPort = -1;
};

#endif //C_UDP_SOCKET_H
