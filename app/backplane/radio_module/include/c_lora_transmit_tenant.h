#ifndef C_LORA_TRANSMIT_TENANT_H
#define C_LORA_TRANSMIT_TENANT_H

#include "n_radio_module_types.h"
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>

#include <f_core/net/device/c_lora.h>
#include <f_core/utils/c_hashmap.h>


class CLoraTransmitTenant : public CTenant {
public:
    explicit CLoraTransmitTenant(const char* name, CLora& lora, CMessagePort<NTypes::RadioBroadcastData>* loraTransmitPort)
        : CTenant(name), lora(lora), loraTransmitPort(*loraTransmitPort)
    {
    }

    ~CLoraTransmitTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    static constexpr uint8_t totalPortsListenedTo = 3;

    CLora& lora;
    CMessagePort<NTypes::RadioBroadcastData>& loraTransmitPort;
    CHashMap<uint16_t, uint8_t[255], totalPortsListenedTo> portDataMap;
};

#endif //C_LORA_TRANSMIT_TENANT_H
