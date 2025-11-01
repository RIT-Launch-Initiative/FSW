#include "c_deployment_module.h"

// F-Core
#include <f_core/messaging/c_msgq_message_port.h>
#include <f_core/net/application/c_sntp_server_tenant.h>
#include <f_core/os/n_rtos.h>
#include <f_core/utils/n_time_utils.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CDeploymentModule);

CDeploymentModule::CDeploymentModule(): CProjectConfiguration() {}

void CDeploymentModule::AddTenantsToTasks() {
}

void CDeploymentModule::AddTasksToRtos() {
}

void CDeploymentModule::SetupCallbacks() {
    alertTenant.Register();
    alertTenant.Subscribe(&pyroControlObserver);

    NTimeUtils::SntpSynchronize(rtc, sntpServerAddr, 5, K_MSEC(100));
}
