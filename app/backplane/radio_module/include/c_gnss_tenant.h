#ifndef C_SENSING_TENANT_H
#define C_SENSING_TENANT_H

#include <n_autocoder_types.h>

#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_runnable_tenant.h>
#include <f_core/utils/c_soft_timer.h>

#include "f_core/radio/c_lora_link.h"


class CGnssTenant : public CRunnableTenant {
public:
    explicit CGnssTenant(const char* name, CMessagePort<LaunchLoraFrame>* loraTransmitPort, CMessagePort<NTypes::GnssData>* dataLoggingPort)
        : CRunnableTenant(name), loraTransmitPort(*loraTransmitPort), dataLoggingPort(*dataLoggingPort)
    {
    }

    ~CGnssTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CSoftTimer transmitTimer{};
    CMessagePort<LaunchLoraFrame>& loraTransmitPort;
    CMessagePort<NTypes::GnssData>& dataLoggingPort;

    void sendGnssToLora() const;
};

#endif //C_SENSING_TENANT_H
