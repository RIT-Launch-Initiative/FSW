#include "c_sensor_module.h"

// F-Core Tenant
#include <f_core/messaging/c_msgq_message_port.h>
#include <f_core/os/n_rtos.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sensor_module);

K_MSGQ_DEFINE(broadcastQueue, sizeof(NTypes::SensorData), 10, 4);
static auto broadcastMsgQueue = CMsgqMessagePort<NTypes::SensorData>(broadcastQueue);

K_MSGQ_DEFINE(dataLogQueue, sizeof(NTypes::SensorData), 10, 4);
static auto dataLogMsgQueue = CMsgqMessagePort<NTypes::SensorData>(dataLogQueue);

CSensorModule::CSensorModule()
    : CProjectConfiguration(), sensorDataBroadcastMessagePort(broadcastMsgQueue),
      sensorDataLogMessagePort(dataLogMsgQueue), flight_log{generateFlightLogPath()} {}

std::string CSensorModule::generateFlightLogPath() {
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
void CSensorModule::AddTenantsToTasks() {
    // Networking
    networkTask.AddTenant(broadcastTenant);
    networkTask.AddTenant(tftpServerTenant);

    // Sensing
    sensingTask.AddTenant(sensingTenant);

    // Data Logging
    dataLogTask.AddTenant(dataLoggerTenant);
}

void CSensorModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkTask);

    // Sensing
    NRtos::AddTask(sensingTask);

    // Data Logging
    NRtos::AddTask(dataLogTask);
}

void CSensorModule::SetupCallbacks() {}
