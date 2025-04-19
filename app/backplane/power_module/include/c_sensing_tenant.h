#ifndef C_SENSING_TENANT_H
#define C_SENSING_TENANT_H

#include <n_autocoder_types.h>

#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>
#include <f_core/utils/c_observer.h>
#include <f_core/utils/c_soft_timer.h>

class CSensingTenant : public CTenant, public CObserver {
public:
    explicit CSensingTenant(const char* name, CMessagePort<NTypes::SensorData> &dataToBroadcast, CMessagePort<NTypes::SensorData> &dataToLog, CMessagePort<NTypes::LoRaBroadcastSensorData> &dataToDownlink)
        : CTenant(name), dataToBroadcast(dataToBroadcast), dataToLog(dataToLog), dataToDownlink(dataToDownlink) {}

    ~CSensingTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

    void Notify(void *ctx) override;

private:
    CMessagePort<NTypes::SensorData> &dataToBroadcast;
    CMessagePort<NTypes::SensorData> &dataToLog;
    CMessagePort<NTypes::LoRaBroadcastSensorData> &dataToDownlink;
    CSoftTimer timer{nullptr, nullptr};

    void sendDownlinkData(const NTypes::SensorData &data);
};


#endif //C_SENSING_TENANT_H
