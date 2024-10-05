#include "receiver/c_radio_receiver.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>

K_MSGQ_DEFINE(broadcastQueue, sizeof(CRadioReceiver::SensorData), 10, 4);
static auto broadcastMsgQueue = CMsgqMessagePort<CRadioReceiver::SensorData>(broadcastQueue);

CRadioReceiver::CRadioReceiver() : CProjectConfiguration(), sensorDataBroadcastMessagePort(broadcastMsgQueue) {
}

void CRadioReceiver::AddTenantsToTasks() {
    // Networking
    networkTask.AddTenant(broadcastTenant);

    // Sensing
    sensingTask.AddTenant(sensingTenant);
}

void CRadioReceiver::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkTask);

    // Sensing
    NRtos::AddTask(sensingTask);
}

void CRadioReceiver::SetupCallbacks() {
}



