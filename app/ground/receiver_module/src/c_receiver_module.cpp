#include "c_receiver_module.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>

#include "f_core/radio/c_lora_link.h"

K_MSGQ_DEFINE(broadcastQueue, 256, 10, 4);
static auto loraBroadcastMsgQueue = CMsgqMessagePort<LaunchLoraFrame>(broadcastQueue);
static auto udpBroadcastMsgQueue = CMsgqMessagePort<LaunchLoraFrame>(broadcastQueue);

CReceiverModule::CReceiverModule() : CProjectConfiguration(), lora(*DEVICE_DT_GET(DT_ALIAS(lora))),
                               loraBroadcastMessagePort(loraBroadcastMsgQueue), udpBroadcastMessagePort(udpBroadcastMsgQueue) {
}

void CReceiverModule::AddTenantsToTasks() {
    // Networking
    networkingTask.AddTenant(commandListenerTenant);
    networkingTask.AddTenant(dataRequestListenerTenant);
    networkingTask.AddTenant(freqChangeTenant);

    // LoRa
    loraTenant.SetToGround();
    loraTenant.RegisterFrameHandler(radioModuleCommandAckPort, freqChangeTenant.AckHandler());
    loraTenant.RegisterDefaultFrameHandler(loraToUdpHandler);
    loraTask.AddTenant(loraTenant);
}

void CReceiverModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkingTask);
    NRtos::AddTask(loraTask);
}

void CReceiverModule::SetupCallbacks() {
}
