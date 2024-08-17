#ifndef C_LORA_TENANT_H
#define C_LORA_TENANT_H

#include "c_radio_module.h"
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>

#include <f_core/net/network/c_ipv4.h>
#include <f_core/net/device/c_lora.h>
#include <f_core/net/transport/c_udp_socket.h>

class CLoraTransmitTenant : public CTenant {
public:
    explicit CLoraTransmitTenant(const char* name, CLora& lora, CMessagePort<CRadioModule::RadioBroadcastData> loraTransmitPort)
        : CTenant(name), lora(lora), loraTransmitPort(loraTransmitPort)
    {
    }

    ~CLoraTransmitTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CLora& lora;
    CMessagePort<CRadioModule::RadioBroadcastData>& loraTransmitPort;
};



#endif //C_LORA_TENANT_H
