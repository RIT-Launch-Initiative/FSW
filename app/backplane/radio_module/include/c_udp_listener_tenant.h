#ifndef C_BROADCAST_TENANT_H
#define C_BROADCAST_TENANT_H

#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>

#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>

namespace NRadioModuleTypes
{
    struct RadioBroadcastData;
}

class CUdpListenerTenant : public CTenant {
public:
    explicit CUdpListenerTenant(const char* name, const char *ipStr, const uint16_t listenPort, CMessagePort<NRadioModuleTypes::RadioBroadcastData>* loraTransmitPort)
        : CTenant(name), ip(CIPv4{ipStr}), udp(CUdpSocket{ip, listenPort, listenPort}),
          loraTransmitPort(*loraTransmitPort), listenPort(listenPort)
    {
    }

    ~CUdpListenerTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CIPv4 ip;
    CUdpSocket udp;
    CMessagePort<NRadioModuleTypes::RadioBroadcastData>& loraTransmitPort;
    const uint16_t listenPort;
};



#endif //C_BROADCAST_TENANT_H
