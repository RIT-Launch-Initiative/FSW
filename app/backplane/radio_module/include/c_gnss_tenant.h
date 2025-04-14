#ifndef C_SENSING_TENANT_H
#define C_SENSING_TENANT_H

#include <n_autocoder_types.h>

#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>
#include <f_core/utils/c_soft_timer.h>


class CGnssTenant : public CTenant {
public:
    explicit CGnssTenant(const char* name, CMessagePort<NTypes::LoRaBroadcastData>* loraTransmitPort, CMessagePort<NTypes::GnssData>* dataLoggingPort)
        : CTenant(name), loraTransmitPort(*loraTransmitPort), dataLoggingPort(*dataLoggingPort)
    {
    }

    ~CGnssTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CSoftTimer transmitTimer{};
    CMessagePort<NTypes::LoRaBroadcastData>& loraTransmitPort;
    CMessagePort<NTypes::GnssData>& dataLoggingPort;
};

#endif //C_SENSING_TENANT_H
