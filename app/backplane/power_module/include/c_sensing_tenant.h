#ifndef C_SENSING_TENANT_H
#define C_SENSING_TENANT_H

#include <n_autocoder_types.h>

#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>
#include <f_core/utils/c_observer.h>

class CSensingTenant : public CTenant, public CObserver {
public:
    explicit CSensingTenant(const char* name, CMessagePort<NTypes::SensorData> &dataToBroadcast, CMessagePort<NTypes::SensorData> &dataToLog)
        : CTenant(name), dataToBroadcast(dataToBroadcast), dataToLog(dataToLog) {}

    ~CSensingTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

    void Notify(void *ctx) override;

private:
    CMessagePort<NTypes::SensorData> &dataToBroadcast;
    CMessagePort<NTypes::SensorData> &dataToLog;
    bool logData = false;
};


#endif //C_SENSING_TENANT_H
