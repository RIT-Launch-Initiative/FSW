#include "c_receiver_module.h"

// F-Core Tenant
#include "f_core/radio/c_lora_link.h"

#include <f_core/messaging/c_msgq_message_port.h>
#include <f_core/os/n_rtos.h>

K_MSGQ_DEFINE(broadcastQueue, sizeof(LaunchLoraFrame), 10, 4);
static auto loraBroadcastMsgQueue = CMsgqMessagePort<LaunchLoraFrame>(broadcastQueue);
static auto udpBroadcastMsgQueue = CMsgqMessagePort<LaunchLoraFrame>(broadcastQueue);

CReceiverModule::CReceiverModule()
    : CProjectConfiguration(),
#ifndef CONFIG_ARCH_POSIX
      lora(*DEVICE_DT_GET(DT_ALIAS(lora))),
#endif
      loraBroadcastMessagePort(loraBroadcastMsgQueue),
      udpBroadcastMessagePort(udpBroadcastMsgQueue) {}

void CReceiverModule::AddTenantsToTasks() {
    // Networking
    networkingTask.AddTenant(commandListenerTenant);
    networkingTask.AddTenant(dataRequestListenerTenant);

#ifndef CONFIG_ARCH_POSIX
    // LoRa
    loraTenant.SetToGround();
    loraTenant.RegisterFrameHandler(radioModuleFrequencyAckPort, freqRequestTenant);
    loraTenant.RegisterDefaultFrameHandler(loraToUdpHandler);
    loraTask.AddTenant(freqRequestTenant);
    loraTask.AddTenant(loraTenant);
#endif
}

void CReceiverModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkingTask);
    NRtos::AddTask(loraTask);
}

void CReceiverModule::SetupCallbacks() {}
