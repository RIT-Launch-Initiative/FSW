#ifdef CONFIG_RADIO_MODULE_RECEIVER

#include "c_receiver_module.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>

K_MSGQ_DEFINE(broadcastQueue, 256, 10, 4);
static auto loraBroadcastMsgQueue = CMsgqMessagePort<NTypes::RadioBroadcastData>(broadcastQueue);
static auto udpBroadcastMsgQueue = CMsgqMessagePort<NTypes::RadioBroadcastData>(broadcastQueue);

CReceiverModule::CReceiverModule() : CProjectConfiguration(), lora(*DEVICE_DT_GET(DT_ALIAS(lora))),
                               loraBroadcastMessagePort(loraBroadcastMsgQueue), udpBroadcastMessagePort(udpBroadcastMsgQueue) {
}

void CReceiverModule::AddTenantsToTasks() {
    // Networking
    networkingTask.AddTenant(commandListenerTenant);

    // LoRa
    loraTask.AddTenant(loraTransmitTenant);
    loraTask.AddTenant(loraReceiveTenant);
}

void CReceiverModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkingTask);
    // NRtos::AddTask(loraTxTask);
    NRtos::AddTask(loraTask);
}

void CReceiverModule::SetupCallbacks() {
}

#endif //CONFIG_RADIO_MODULE_RECEIVER