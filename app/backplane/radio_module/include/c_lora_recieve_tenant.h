#ifndef C_LORA_RECEIVE_TENANT_H
#define C_LORA_RECEIVE_TENANT_H

#include <f_core/os/c_tenant.h>

#include <f_core/net/device/c_lora.h>
#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/device/c_gpio.h>

class CLoraRecieveTenant : public CTenant {
public:
    explicit CLoraRecieveTenant(const char* name, CLora& lora, const char* ip, const uint16_t srcPort)
        : CTenant(name), lora(lora), udp(CUdpSocket(CIPv4(ip), srcPort, srcPort)) {}

    ~CLoraRecieveTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CLora& lora;
    CUdpSocket udp;
    const CGpio gpios[4] = {CGpio(*DEVICE_DT_GET(DT_ALIAS(gpios0))), CGpio(*DEVICE_DT_GET(DT_ALIAS(gpios1))),
                CGpio(*DEVICE_DT_GET(DT_ALIAS(gpios2))), CGpio(*DEVICE_DT_GET(DT_ALIAS(gpios3)))};
};



#endif //C_LORA_RECEIVE_TENANT_H
