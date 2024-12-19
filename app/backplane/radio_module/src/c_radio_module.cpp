#ifndef CONFIG_RADIO_MODULE_RECEIVER

#include "c_radio_module.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>
#include <zephyr/drivers/gnss.h>

K_MSGQ_DEFINE(broadcastQueue, 256, 10, 4);
static auto loraBroadcastMsgQueue = CMsgqMessagePort<NTypes::RadioBroadcastData>(broadcastQueue);
static auto udpBroadcastMsgQueue = CMsgqMessagePort<NTypes::RadioBroadcastData>(broadcastQueue);

CRadioModule::CRadioModule() : CProjectConfiguration(), lora(*DEVICE_DT_GET(DT_ALIAS(lora))),
                               loraBroadcastMessagePort(loraBroadcastMsgQueue), udpBroadcastMessagePort(udpBroadcastMsgQueue) {
}

void CRadioModule::AddTenantsToTasks() {
    // Networking
    networkingTask.AddTenant(sensorModuleListenerTenant);
    networkingTask.AddTenant(powerModuleListenerTenant);

    // LoRa
    loraTask.AddTenant(loraTransmitTenant);
}

void CRadioModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkingTask);
    NRtos::AddTask(loraTask);
}

void CRadioModule::SetupCallbacks() {
}

#endif //CONFIG_RADIO_MODULE_RECEIVER