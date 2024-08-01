#ifndef C_BROADCAST_TENANT_H
#define C_BROADCAST_TENANT_H

#include <f_core/os/c_tenant.h>

#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>

class CBroadcastTenant : public CTenant {
public:
    explicit CBroadcastTenant(const char* name)
        : CTenant(name)
    {
    }

    ~CBroadcastTenant() override = default;

    void Startup() override = delete;

    void PostStartup() override = delete;

    void Run() override;

private:
    CIPv4 ip{"10.0.0.0"};
    CUdpSocket udp{ip, 10000, 10000};
};



#endif //C_BROADCAST_TENANT_H
