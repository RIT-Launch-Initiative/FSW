#ifndef C_UDP_LISTENER_TENANT_H
#define C_UDP_LISTENER_TENANT_H

#include <n_autocoder_types.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_runnable_tenant.h>

#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>

class CUdpListenerTenant : public CRunnableTenant {
public:
    explicit CUdpListenerTenant(const char* name, const char *ipStr, const uint16_t listenPort, CMessagePort<NTypes::LoRaBroadcastData>* loraTransmitPort)
        : CRunnableTenant(name), ip(CIPv4{ipStr}), udp(CUdpSocket{ip, listenPort, listenPort}),
          loraTransmitPort(*loraTransmitPort), listenPort(listenPort) {}

    ~CUdpListenerTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CIPv4 ip;
    CUdpSocket udp;
    CMessagePort<NTypes::LoRaBroadcastData>& loraTransmitPort;
    const uint16_t listenPort;
};

#endif //C_UDP_LISTENER_TENANT_H
