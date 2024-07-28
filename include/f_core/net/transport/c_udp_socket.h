#ifndef C_UDP_SOCKET_H
#define C_UDP_SOCKET_H

#include <cstdint>
#include <f_core/net/c_transciever.h>

class CIPv4;

class CUdpSocket : public CTransceiver {
public:
    CUdpSocket(CIPv4 &ipv4, uint16_t srcPort, uint16_t dstPort);

    ~CUdpSocket() = default;

    int TransmitSynchronous(const void *data, size_t len) override;

    int ReceiveSynchronous(void *data, size_t len) override;

    int TransmitAsynchronous(const void *data, size_t len) override;

    int ReceiveAsynchronous(void *data, size_t len) override;

    int SetRxTimeout(int timeout) override;


private:
    int sock = -1;
    int dstPort = -1;
};

#endif //C_UDP_SOCKET_H
