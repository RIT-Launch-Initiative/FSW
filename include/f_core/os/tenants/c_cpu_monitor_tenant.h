#ifndef C_CPU_MONITOR_TENANT_H
#define C_CPU_MONITOR_TENANT_H

#include "f_core/os/c_tenant.h"


class CCpuMonitorTenant : public CTenant {
public:
    static CCpuMonitorTenant &GetInstance() {
        return instance;
    }
    
    CCpuMonitorTenant (const CCpuMonitorTenant&) = delete;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

    void Cleanup() override;
    
private:
    CCpuMonitorTenant();
    static CCpuMonitorTenant instance;
};

#endif //C_CPU_MONITOR_TENANT_H
