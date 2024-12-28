#ifndef C_COMMAND_LISTENER_TENANT
#define C_COMMAND_LISTENER_TENANT

#include "n_radio_module_types.h"

#include <f_core/os/c_tenant.h>
#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/device/c_gpio.h>

class CCommandListenerTenant : public CTenant {
public:
    explicit CCommandListenerTenant(const char* name, const char* ipStr, const uint16_t listenPort)
    : CTenant(name), ip(CIPv4{ip}), udp(CUdpSocket{ip, listenPort, listenPort}),
        gpios{CGpio(*DEVICE_DT_GET(DT_ALIAS(gpios1))), CGpio(*DEVICE_DT_GET(DT_ALIAS(gpios2))),
                CGpio(*DEVICE_DT_GET(DT_ALIAS(gpios3))), CGpio(*DEVICE_DT_GET(DT_ALIAS(gpios4)))} {}

    ~CCommandListenerTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CIPv4 ip;
    CUdpSocket udp;
    const uint16_t listenPort;
    const CGpio gpios[4];
    
};

#endif // C_COMMAND_LISTENER_TENANT