#include "c_power_module.h"

#include <n_autocoder_types.h>

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>
#include <f_core/utils/n_time_utils.h>

K_MSGQ_DEFINE(broadcastQueue, sizeof(NTypes::SensorData), 10, 4);
static auto broadcastMsgQueue = CMsgqMessagePort<NTypes::SensorData>(broadcastQueue);

K_MSGQ_DEFINE(dataLogQueue, sizeof(NTypes::SensorData), 10, 4);
static auto dataLogMsgQueue = CMsgqMessagePort<NTypes::SensorData>(dataLogQueue);

K_MSGQ_DEFINE(downlinkQueue, sizeof(NTypes::LoRaBroadcastSensorData), 10, 4);
static auto downlinkMessageQueue = CMsgqMessagePort<NTypes::LoRaBroadcastSensorData>(downlinkQueue);

CPowerModule::CPowerModule() : CProjectConfiguration(), sensorDataBroadcastMessagePort(broadcastMsgQueue),
                               sensorDataLogMessagePort(dataLogMsgQueue),
                               sensorDataDownlinkMessagePort(downlinkMessageQueue) {}

void CPowerModule::AddTenantsToTasks() {
    // Networking
    networkTask.AddTenant(broadcastTenant);
    networkTask.AddTenant(alertTenant);

    // Sensing
    sensingTask.AddTenant(sensingTenant);

    // Data Logging
    dataLoggingTask.AddTenant(dataLoggerTenant);
}

void CPowerModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkTask);

    // Sensing
    NRtos::AddTask(sensingTask);

    // Data Logging
    NRtos::AddTask(dataLoggingTask);
}

void CPowerModule::SetupCallbacks() {
    alertTenant.Subscribe(&sensingTenant);

    // Not a callback, but ¯\_(ツ)_/¯
    // Maybe have Add and Setup tasks be private and have main.cpp call a single function?
    // Configuration children would call CBase::Setup and then can add their own setup below
    NTimeUtils::SntpSynchronize(rtc, sntpServerAddr, 5, K_MSEC(100));
}

void CPowerModule::Cleanup() {
    dataLoggerTenant.Cleanup();
}

