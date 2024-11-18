#ifdef CONFIG_RADIO_MODULE_RECEIVER

#include "c_receiver_module.h"

// F-Core Tenant
#include <f_core/messaging/c_msgq_message_port.h>
#include <f_core/os/n_rtos.h>

K_MSGQ_DEFINE(broadcastQueue, 256, 10, 4);
static auto loraBroadcastMsgQueue = CMsgqMessagePort<NRadioModuleTypes::RadioBroadcastData>(broadcastQueue);
static auto udpBroadcastMsgQueue = CMsgqMessagePort<NRadioModuleTypes::RadioBroadcastData>(broadcastQueue);

CReceiverModule::CReceiverModule()
    : CProjectConfiguration(), lora(*DEVICE_DT_GET(DT_ALIAS(lora))), loraBroadcastMessagePort(loraBroadcastMsgQueue),
      udpBroadcastMessagePort(udpBroadcastMsgQueue) {}

void CReceiverModule::AddTenantsToTasks() {
    // Networking
    networkingTask.AddTenant(commandListenerTenant);

    // LoRa
    loraTxTask.AddTenant(loraTransmitTenant);
    loraRxTask.AddTenant(loraReceiveTenant);
}

void CReceiverModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkingTask);
    // NRtos::AddTask(loraTxTask);
    NRtos::AddTask(loraRxTask);
}

void CReceiverModule::SetupCallbacks() {}

#endif //CONFIG_RADIO_MODULE_RECEIVER