#ifndef C_LORA_RECEIVE_TENANT_H
#define C_LORA_RECEIVE_TENANT_H

#include "n_radio_module_types.h"
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>

#include <f_core/net/device/c_lora.h>

class CLoraReceiveTenant : public CTenant {
public:
    explicit CLoraReceiveTenant(const char* name, CLora& lora, CMessagePort<NRadioModuleTypes::RadioBroadcastData>* udpTransmitPort)
        : CTenant(name), lora(lora), udpTransmitPort(*udpTransmitPort)
    {
    }

    ~CLoraReceiveTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CLora& lora;
    CMessagePort<NRadioModuleTypes::RadioBroadcastData>& udpTransmitPort;
};



#endif //C_LORA_RECEIVE_TENANT_H
