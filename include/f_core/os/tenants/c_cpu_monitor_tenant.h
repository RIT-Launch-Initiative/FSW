#ifndef C_CPU_MONITOR_TENANT_H
#define C_CPU_MONITOR_TENANT_H

#include "f_core/messaging/c_message_port.h"
#include "f_core/os/c_tenant.h"


class CCpuMonitorTenant : public CTenant {
public:
    CCpuMonitorTenant(const CMessagePort<NTypes::CPUMonitor> &outputPort) :
        CTenant("CPU Monitor Tenant"), outputPort(outputPort) {};

    void Startup() override;

    void PostStartup() override;

    void Run() override;

    void Cleanup() override;
    
private:
    static CCpuMonitorTenant instance;
    CMessagePort<NTypes::CPUMonitor> &outputPort;
};

#endif //C_CPU_MONITOR_TENANT_H
