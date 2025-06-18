#include "f_core/os/tenants/c_cpu_monitor_tenant.h"


CCpuMonitorTenant CCpuMonitorTenant::instance;

CCpuMonitorTenant::CCpuMonitorTenant() : ::CTenant("CPU Monitor Tenant") {}

void CCpuMonitorTenant::Startup() {}
void CCpuMonitorTenant::PostStartup() {}
void CCpuMonitorTenant::Run() {}
void CCpuMonitorTenant::Cleanup() {}
