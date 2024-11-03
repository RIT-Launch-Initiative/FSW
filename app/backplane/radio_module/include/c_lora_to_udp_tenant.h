#ifndef C_LORA_RECEIVE_TENANT_H
#define C_LORA_RECEIVE_TENANT_H

#include <f_core/os/c_tenant.h>

#include <f_core/net/device/c_lora.h>
#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>

class CLoraToUdpTenant : public CTenant {
public:
    explicit CLoraToUdpTenant(const char* name, CLora& lora, const char* ip, const uint16_t srcPort)
        : CTenant(name), lora(lora), udp(CUdpSocket(CIPv4(ip), srcPort, srcPort))
    {
    }

    ~CLoraToUdpTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CLora& lora;
    CUdpSocket udp;
};



#endif //C_LORA_RECEIVE_TENANT_H
