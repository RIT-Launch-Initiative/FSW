#ifndef C_SENSING_TENANT_H
#define C_SENSING_TENANT_H

#include <n_autocoder_types.h>

#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>

# TODO: Replace with Adc instead of Sensing

class CSensingTenant : public CTenant {
public:
    explicit CSensingTenant(const char* name, CMessagePort<NTypes::SensorData> &dataToBroadcast, CMessagePort<NTypes::SensorData> &dataToLog)
        : CTenant(name), dataToBroadcast(dataToBroadcast), dataToLog(dataToLog)
    {
    }

    ~CSensingTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override; // TODO: read from c_adc.cpp, use that math thing from the doc schematic

private:
    CMessagePort<NTypes::SensorData> &dataToBroadcast;
    CMessagePort<NTypes::SensorData> &dataToLog;
};


#endif //C_SENSING_TENANT_H
