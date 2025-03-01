#include "c_sensor_module.h"

// F-Core Tenant
#include <f_core/messaging/c_msgq_message_port.h>
#include <f_core/os/n_rtos.h>

K_MSGQ_DEFINE(broadcastQueue, sizeof(NTypes::SensorData), 10, 4);
static auto broadcastMsgQueue = CMsgqMessagePort<NTypes::SensorData>(broadcastQueue);

K_MSGQ_DEFINE(dataLogQueue, sizeof(NTypes::SensorData), 10, 4);
static auto dataLogMsgQueue = CMsgqMessagePort<NTypes::SensorData>(dataLogQueue);

CSensorModule::CSensorModule()
    : CProjectConfiguration(), sensorDataBroadcastMessagePort(broadcastMsgQueue),
      sensorDataLogMessagePort(dataLogMsgQueue) {}

void CSensorModule::AddTenantsToTasks() {
    // Networking
    networkTask.AddTenant(broadcastTenant);

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
