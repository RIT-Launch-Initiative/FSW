#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include "c_sensing_tenant.h"
#include "c_broadcast_tenant.h"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/os/c_task.h>
#include <f_core/os/c_tenant.h>

class CSensorModule : public CProjectConfiguration {
public:
    CSensorModule();

private:
    // Tenants
    CSensingTenant sensingTenant{"Sensing Tenant"};
    CBroadcastTenant broadcastTenant{"Broadcast Tenant"};

    // Tasks
    CTask networkTask{"Networking Task", 15, 128, 0};
    CTask sensingTask{"Sensing Task", 15, 128, 0};

    void addTenants() override;

    void addTasks() override;

    void setupMessagePorts() override {};

    void setupCallbacks() override {};
};



#endif //C_SENSOR_MODULE_H
