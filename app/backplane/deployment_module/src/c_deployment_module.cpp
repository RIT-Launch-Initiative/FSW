#include "c_deployment_module.h"

// F-Core
#include <f_core/messaging/c_msgq_message_port.h>
#include <f_core/net/application/c_sntp_server_tenant.h>
#include <f_core/os/n_rtos.h>
#include <f_core/utils/n_time_utils.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CDeploymentModule);

CDeploymentModule::CDeploymentModule(): CProjectConfiguration(), flight_log{generateFlightLogPath()} {}

std::string CDeploymentModule::generateFlightLogPath() {
    constexpr size_t MAX_FLIGHT_LOG_PATH_SIZE = 32;
    char flightLogPath[MAX_FLIGHT_LOG_PATH_SIZE] = {0};
    for (size_t i = 0; i < 100; i++) {
        snprintf(flightLogPath, MAX_FLIGHT_LOG_PATH_SIZE, "/lfs/flight_log%02d.txt", i);
        struct fs_dirent ent;
        int ret = fs_stat(flightLogPath, &ent);
        if (ret == -ENOENT) {
            // This path works
            LOG_INF("Using %s for flight log", flightLogPath);
            break;
        } else if (ret != 0) {
            LOG_WRN("Error reading filesystem for flight log. (error %d) Using name %s", ret, flightLogPath);
            break;
        }
        // otherwise, keep counting up
    }
    std::string copystr{flightLogPath};
    return copystr;
}
void CDeploymentModule::AddTenantsToTasks() {
    // Networking

    // Data Logging
}

void CDeploymentModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkTask);
}

void CDeploymentModule::SetupCallbacks() {
    NTimeUtils::SntpSynchronize(rtc, sntpServerAddr, 5, K_MSEC(100));
}
