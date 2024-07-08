#ifndef C_UDP_SOCKET_H
#define C_UDP_SOCKET_H

#include <cstdint>
#include <f_core/net/c_transciever.h>
#include <f_core/net/network/c_ipv4.h>


class CUdpSocket : public CTransceiver {
    CUdpSocket(CIPv4 &ipv4, uint16_t srcPort);

    int TransmitSynchronous(const void *data, size_t len) override;

    int ReceiveSynchronous(void *data, size_t len) override;

    int TransmitAsynchronous(const void *data, size_t len) override;

    int ReceiveAsynchronous(void *data, size_t len) override;
};



#endif //C_UDP_SOCKET_H
