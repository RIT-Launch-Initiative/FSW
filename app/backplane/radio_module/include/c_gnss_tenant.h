#ifndef C_SENSING_TENANT_H
#define C_SENSING_TENANT_H

#include <n_radio_module_types.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>
#include <f_core/utils/c_soft_timer.h>


class CGnssTenant : public CTenant {
public:
    explicit CGnssTenant(const char* name, CMessagePort<NRadioModuleTypes::RadioBroadcastData>* loraTransmitPort)
        : CTenant(name), loraTransmitPort(*loraTransmitPort)
    {
    }

    ~CGnssTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CSoftTimer transmitTimer{};
    CMessagePort<NRadioModuleTypes::RadioBroadcastData>& loraTransmitPort;
};

#endif //C_SENSING_TENANT_H
