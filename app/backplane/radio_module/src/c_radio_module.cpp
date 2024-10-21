#include "c_radio_module.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>

K_MSGQ_DEFINE(broadcastQueue, 256, 10, 4);
static auto broadcastMsgQueue = CMsgqMessagePort<NRadioModuleTypes::RadioBroadcastData>(broadcastQueue);

CRadioModule::CRadioModule() : CProjectConfiguration(), lora(*DEVICE_DT_GET(DT_ALIAS(lora))), loraBroadcastMessagePort(broadcastMsgQueue) {
}

void CRadioModule::AddTenantsToTasks() {
    // Networking
    networkTask.AddTenant(broadcastReceiveTenant);

    // GNSS
    gnssTask.AddTenant(gnssTenant);

    // LoRa
    loraTask.AddTenant(loraTransmitTenant);
}

void CRadioModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkTask);
}

void CRadioModule::SetupCallbacks() {
}



