#include "c_power_module.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>

K_MSGQ_DEFINE(broadcastQueue, sizeof(CPowerModule::SensorData), 10, 4);
static auto broadcastMsgQueue = CMsgqMessagePort<CPowerModule::SensorData>(broadcastQueue);

CPowerModule::CPowerModule() : CProjectConfiguration(), sensorDataBroadcastMessagePort(broadcastMsgQueue) {
}

void CPowerModule::AddTenantsToTasks() {
    // Networking
    networkTask.AddTenant(broadcastTenant);

    // Sensing
    sensingTask.AddTenant(sensingTenant);
}

void CPowerModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkTask);

    // Sensing
    NRtos::AddTask(sensingTask);
}

void CPowerModule::SetupCallbacks() {
}



