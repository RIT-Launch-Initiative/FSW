#ifndef C_POWER_MODULE_CONFIGURATION_H
#define C_POWER_MODULE_CONFIGURATION_H

#include <f_core/c_project_configuration.h>
#include <f_core/os/c_task.h>
#include <f_core/os/c_tenant.h>

class CPowerModuleConfiguration : public CProjectConfiguration {
public:
    CPowerModuleConfiguration();

    void SetupTasks() override;

private:
    // Tasks
    CTask powerMonitorTask;
    CTask ethernetTask;
    CTask loggingTask;
};

#endif //C_POWER_MODULE_CONFIGURATION_H
