#include "c_deployment_module.h"

// F-Core
#include <f_core/messaging/c_msgq_message_port.h>
#include <f_core/os/n_rtos.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CDeploymentModule);

CDeploymentModule::CDeploymentModule(): CProjectConfiguration() {}

void CDeploymentModule::AddTenantsToTasks() {
    // Networking
    networkTask.AddTenant(alertTenant);
}

void CDeploymentModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkTask);
}

void CDeploymentModule::SetupCallbacks() {
    alertTenant.Subscribe(&pyroControlObserver);
}
