#include "transmitter/c_radio_transmitter.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>

K_MSGQ_DEFINE(broadcastQueue, sizeof(CRadioTransmitter::SensorData), 10, 4);
static auto broadcastMsgQueue = CMsgqMessagePort<CRadioTransmitter::SensorData>(broadcastQueue);

CRadioTransmitter::CRadioTransmitter() : CProjectConfiguration() {
}

void CRadioTransmitter::AddTenantsToTasks() {
    // Networking
    networkTask.AddTenant(broadcastTenant);
}

void CRadioTransmitter::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkTask);

    // Sensing
    NRtos::AddTask(sensingTask);
}

void CRadioTransmitter::SetupCallbacks() {
}



