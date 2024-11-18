#ifndef C_LORA_TRANSMIT_TENANT_H
#define C_LORA_TRANSMIT_TENANT_H

#include "n_radio_module_types.h"
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>

#include <f_core/net/device/c_lora.h>

class CLoraTransmitTenant : public CTenant {
public:
    explicit CLoraTransmitTenant(const char* name, CLora& lora, CMessagePort<NRadioModuleTypes::RadioBroadcastData>* loraTransmitPort)
        : CTenant(name), lora(lora), loraTransmitPort(*loraTransmitPort)
    {
    }

    ~CLoraTransmitTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CLora& lora;
    CMessagePort<NRadioModuleTypes::RadioBroadcastData>& loraTransmitPort;
};

#endif //C_LORA_TRANSMIT_TENANT_H
