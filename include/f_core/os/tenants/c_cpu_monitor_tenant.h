#ifndef C_CPU_MONITOR_TENANT_H
#define C_CPU_MONITOR_TENANT_H

#include "f_core/messaging/c_message_port.h"
#include "f_core/os/c_tenant.h"

#include <stdint.h>

struct __attribute__((packed)) CpuMonitorData {
    uint32_t Uptime;
    int32_t DieTemperature;
    uint8_t Utilization;
};

class CCpuMonitorTenant : public CTenant {
public:
    CCpuMonitorTenant(CMessagePort<CpuMonitorData>& outputPort) :
        CTenant("CPU Monitor Tenant"), outputPort(outputPort) {
    };

    void Startup() override;

    void PostStartup() override;

    void Run() override;

    void Cleanup() override;

private:
    CMessagePort<CpuMonitorData>& outputPort;
    uint64_t prevExecutionCycles = 0;
    uint64_t prevTotalCycles = 0;

#if DT_NODE_EXISTS(DT_ALIAS(die_temp))
    const device *dieTempSensor = DEVICE_DT_GET(DT_ALIAS(die_temp));
#endif

    uint8_t getUtilization();

    uint32_t getUptime();

    int32_t getDieTemperature();
};

#endif //C_CPU_MONITOR_TENANT_H
