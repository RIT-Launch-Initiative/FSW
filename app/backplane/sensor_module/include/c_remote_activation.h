#ifndef C_REMOTE_ACTIVATION_TENANT_H
#define C_REMOTE_ACTIVATION_TENANT_H

#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/os/c_tenant.h>
#include <zephyr/drivers/gpio.h>

class CRemoteActivationTenant : public CTenant {
public:
    explicit CRemoteActivationTenant(const char* name, const gpio_dt_spec &pin)
        : CTenant(name), ip(CIPv4{"10.3.2.1"}), udp(CUdpSocket{ip, commandPort, commandPort}), pin(pin)
    {
    }

    ~CRemoteActivationTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    static constexpr uint16_t commandPort = 13001;

    CIPv4 ip;
    CUdpSocket udp;
    const gpio_dt_spec &pin;
};


#endif //C_REMOTE_ACTIVATION_TENANT_H
