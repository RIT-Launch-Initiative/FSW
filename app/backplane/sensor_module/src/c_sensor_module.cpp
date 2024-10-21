#include "c_sensor_module.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>

K_MSGQ_DEFINE(broadcastQueue, sizeof(CSensorModule::SensorData), 10, 4);
static auto broadcastMsgQueue = CMsgqMessagePort<CSensorModule::SensorData>(broadcastQueue);

CSensorModule::CSensorModule() : CProjectConfiguration(), sensorDataBroadcastMessagePort(broadcastMsgQueue) {
}

void CSensorModule::AddTenantsToTasks() {
    // Networking
    networkTask.AddTenant(broadcastTenant);

    // Sensing
    sensingTask.AddTenant(sensingTenant);
}

void CSensorModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkTask);

    // Sensing
    NRtos::AddTask(sensingTask);
}

void CSensorModule::SetupCallbacks() {
}



