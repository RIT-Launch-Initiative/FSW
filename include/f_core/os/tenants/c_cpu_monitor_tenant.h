#ifndef C_CPU_MONITOR_TENANT_H
#define C_CPU_MONITOR_TENANT_H

#include "f_core/messaging/c_message_port.h"
#include "f_core/os/c_tenant.h"

#include <FSW/n_autocoder_types.h>

class CCpuMonitorTenant : public CTenant {
public:
    CCpuMonitorTenant(CMessagePort<NTypes::CPUMonitor> &outputPort) :
        CTenant("CPU Monitor Tenant"), outputPort(outputPort) {};

    void Startup() override;

    void PostStartup() override;

    void Run() override;

    void Cleanup() override;
    
private:
    static CCpuMonitorTenant instance;
    CMessagePort<NTypes::CPUMonitor> &outputPort;

    uint32_t prevExecutionCycles = 0;
    uint32_t prevTotalCycles = 0;

#if DT_NODE_EXISTS(DT_ALIAS(die_temp))
    const device *dieTempSensor = DEVICE_DT_GET(DT_ALIAS(die_temp));
#endif

    uint8_t getUtilization();

    uint32_t getUptime();

    int32_t getDieTemperature();
};

#endif //C_CPU_MONITOR_TENANT_H
