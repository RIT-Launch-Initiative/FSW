#ifndef C_BROADCAST_TENANT_H
#define C_BROADCAST_TENANT_H

#include <f_core/os/c_tenant.h>

#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>

class CBroadcastReceiveTenant : public CTenant {
public:
    explicit CBroadcastReceiveTenant(const char* name, const char *ipStr, const uint16_t listenPorts)
        : CTenant(name), ip(CIPv4{ipStr}), udp(CUdpSocket{ip, listenPort, listenPort})
    {
    }

    ~CBroadcastReceiveTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CIPv4 ip;
    CUdpSocket udp;
};



#endif //C_BROADCAST_TENANT_H
