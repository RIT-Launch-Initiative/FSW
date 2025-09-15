#ifndef C_SENSING_TENANT_H
#define C_SENSING_TENANT_H

#include "f_core/device/c_rtc.h"

#include <n_autocoder_types.h>

#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>
#include <f_core/utils/c_observer.h>
#include <f_core/utils/c_soft_timer.h>

class CSensingTenant : public CTenant, public CObserver {
public:
    explicit CSensingTenant(const char* name, CMessagePort<NTypes::TimestampedSensorData> &sensorMessagePort)
        : CTenant(name), sensorMessagePort(sensorMessagePort) {}

    ~CSensingTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

    void Notify(void *ctx) override;

private:
    CMessagePort<NTypes::TimestampedSensorData> &sensorMessagePort;
    CSoftTimer timer{nullptr, nullptr};
    CRtc rtc{*DEVICE_DT_GET(DT_ALIAS(rtc))};
};


#endif //C_SENSING_TENANT_H
