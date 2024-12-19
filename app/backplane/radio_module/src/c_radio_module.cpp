#ifndef CONFIG_RADIO_MODULE_RECEIVER

#include "c_radio_module.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>
#include <zephyr/drivers/gnss.h>

K_MSGQ_DEFINE(loraBroadcastQueue, 256, 10, 4);
K_MSGQ_DEFINE(udpBroadcastQueue, 256, 10, 4);
K_MSGQ_DEFINE(gnssDataLogQueue, 256, 10, 4);
static auto loraBroadcastMsgQueue = CMsgqMessagePort<NTypes::RadioBroadcastData>(loraBroadcastQueue);
static auto udpBroadcastMsgQueue = CMsgqMessagePort<NTypes::RadioBroadcastData>(udpBroadcastQueue);
static auto gnssBroadcastMsgQueue = CMsgqMessagePort<NTypes::GnssBroadcastData>(gnssDataLogQueue);

CRadioModule::CRadioModule() : CProjectConfiguration(), lora(*DEVICE_DT_GET(DT_ALIAS(lora))),
                               loraBroadcastMessagePort(loraBroadcastMsgQueue), udpBroadcastMessagePort(udpBroadcastMsgQueue), gnssDataLogMessagePort(gnssBroadcastMsgQueue) {
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