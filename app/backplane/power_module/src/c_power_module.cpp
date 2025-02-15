#include "c_power_module.h"

#include <n_autocoder_types.h>

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>

// Sensing
K_MSGQ_DEFINE(sensorBroadcastQueue, sizeof(NTypes::SensorData), 10, 4);
static auto sensorBroadcastMsgQueue = CMsgqMessagePort<NTypes::SensorData>(sensorBroadcastQueue);

K_MSGQ_DEFINE(sensorDataLogQueue, sizeof(NTypes::SensorData), 10, 4);
static auto sensorDataLogMsgQueue = CMsgqMessagePort<NTypes::SensorData>(sensorDataLogQueue);

// ADC
K_MSGQ_DEFINE(adcBroadcastQueue, sizeof(int32_t), 10, 4);
static auto adcBroadcastMsgQueue = CMsgqMessagePort<int32_t>(adcBroadcastQueue);

K_MSGQ_DEFINE(adcDataLogQueue, sizeof(int32_t), 10, 4);
static auto adcDataLogMsgQueue = CMsgqMessagePort<int32_t>(adcDataLogQueue);

CPowerModule::CPowerModule() : CProjectConfiguration(),
sensorDataBroadcastMessagePort(sensorBroadcastMsgQueue), sensorDataLogMessagePort(sensorDataLogMsgQueue),
adcDataBroadcastMessagePort(adcBroadcastMsgQueue), adcDataLogMessagePort(adcDataLogMsgQueue) {}

void CPowerModule::AddTenantsToTasks() {
    // Networking
    networkTask.AddTenant(sensorBroadcastTenant);
    networkTask.AddTenant(adcBroadcastTenant);

    // Sensing and ADC
    sensingTask.AddTenant(sensingTenant);
    sensingTask.AddTenant(adcTenant);

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
}

void CPowerModule::Cleanup() {
    dataLoggerTenant.Cleanup();
}

