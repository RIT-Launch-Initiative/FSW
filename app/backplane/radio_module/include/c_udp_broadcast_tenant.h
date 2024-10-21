#ifndef C_UDP_BROADCAST_TENANT_H
#define C_UDP_BROADCAST_TENANT_H

#include "n_radio_module_types.h"
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>

#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>

class CUdpBroadcastTenant : public CTenant {
public:
    explicit CUdpBroadcastTenant(const char* name, const char *ipStr, CMessagePort<NRadioModuleTypes::RadioBroadcastData>* loraTransmitPort)
        : CTenant(name), ip(CIPv4{ipStr}), udp(CUdpSocket{ip, listenPort, listenPort}),
          loraTransmitPort(*loraTransmitPort), listenPort(listenPort)
    {
    }

    ~CUdpBroadcastTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CIPv4 ip;
    CUdpSocket udp;
    CMessagePort<NRadioModuleTypes::RadioBroadcastData>& loraTransmitPort;
};



#endif //C_UDP_BROADCAST_TENANT_H
