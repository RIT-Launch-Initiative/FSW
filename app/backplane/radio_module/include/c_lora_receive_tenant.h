#ifndef C_LORA_RECEIVE_TENANT_H
#define C_LORA_RECEIVE_TENANT_H

#include "n_radio_module_types.h"

#include <f_core/os/c_tenant.h>
#include <f_core/net/device/c_lora.h>
#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/device/c_gpio.h>

class CLoraReceiveTenant : public CTenant {
public:
    explicit CLoraReceiveTenant(const char* name, CLora& lora, const char* ip, const uint16_t srcPort, CMessagePort<NTypes::RadioBroadcastData>* loraTransmitPort)
        : CTenant(name), lora(lora), udp(CUdpSocket(CIPv4(ip), srcPort, srcPort)), loraTransmitPort(*loraTransmitPort) {}

    ~CLoraReceiveTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CLora& lora;
    CUdpSocket udp;
    CGpio gpios[4] = {CGpio(*DEVICE_DT_GET(DT_ALIAS(gpio0))), CGpio(*DEVICE_DT_GET(DT_ALIAS(gpio1))),
                CGpio(*DEVICE_DT_GET(DT_ALIAS(gpio2))), CGpio(*DEVICE_DT_GET(DT_ALIAS(gpio3)))};
    CMessagePort<NTypes::RadioBroadcastData>& loraTransmitPort;
};



#endif //C_LORA_RECEIVE_TENANT_H
