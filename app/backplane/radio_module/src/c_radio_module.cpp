#include "c_radio_module.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>
#include <zephyr/drivers/gnss.h>

K_MSGQ_DEFINE(broadcastQueue, 256, 10, 4);
static auto loraBroadcastMsgQueue = CMsgqMessagePort<NRadioModuleTypes::RadioBroadcastData>(broadcastQueue);
static auto udpBroadcastMsgQueue = CMsgqMessagePort<NRadioModuleTypes::RadioBroadcastData>(broadcastQueue);

CRadioModule::CRadioModule() : CProjectConfiguration(), lora(*DEVICE_DT_GET(DT_ALIAS(lora))),
                               loraBroadcastMessagePort(loraBroadcastMsgQueue), udpBroadcastMessagePort(udpBroadcastMsgQueue) {
}

void CRadioModule::AddTenantsToTasks() {
    // Networking
    networkingTask.AddTenant(sensorModuleListenerTenant);
    networkingTask.AddTenant(powerModuleListenerTenant);

    // GNSS
    gnssTask.AddTenant(gnssTenant);

    // LoRa
    loraTask.AddTenant(loraTransmitTenant);
}

void CRadioModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkingTask);
    NRtos::AddTask(gnssTask);
    NRtos::AddTask(loraTask);
}

void CRadioModule::SetupCallbacks() {
}



