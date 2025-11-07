#include "c_radio_module.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>
#include <zephyr/drivers/gnss.h>

K_MSGQ_DEFINE(loraBroadcastQueue, sizeof(NTypes::LoRaBroadcastData), 10, 4);
K_MSGQ_DEFINE(udpBroadcastQueue, sizeof(NTypes::LoRaBroadcastData), 10, 4);
K_MSGQ_DEFINE(gnssDataLogQueue, sizeof(NTypes::GnssData), 10, 4);
static auto loraBroadcastMsgQueue = CMsgqMessagePort<NTypes::LoRaBroadcastData>(loraBroadcastQueue);
static auto udpBroadcastMsgQueue = CMsgqMessagePort<NTypes::LoRaBroadcastData>(udpBroadcastQueue);
static auto gnssLogMsgQueue = CMsgqMessagePort<NTypes::GnssData>(gnssDataLogQueue);

CRadioModule::CRadioModule() : CProjectConfiguration(),
#ifndef CONFIG_ARCH_POSIX
                               lora(*DEVICE_DT_GET(DT_ALIAS(lora))),
#endif
                               loraBroadcastMessagePort(loraBroadcastMsgQueue),
                               udpBroadcastMessagePort(udpBroadcastMsgQueue), gnssDataLogMessagePort(gnssLogMsgQueue) {}

void CRadioModule::AddTenantsToTasks() {
    // Networking
    networkingTask.AddTenant(sensorModuleListenerTenant);
    networkingTask.AddTenant(powerModuleListenerTenant);

#ifndef CONFIG_ARCH_POSIX
    // LoRa
    loraTask.AddTenant(loraTransmitTenant);
#endif
    // Data Logging
    dataLoggingTask.AddTenant(dataLoggerTenant);

    // GNSS
    gnssTask.AddTenant(gnssTenant);
}

void CRadioModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkingTask);
    NRtos::AddTask(loraTask);

    // Data Logging
    NRtos::AddTask(dataLoggingTask);

    // GNSS
    NRtos::AddTask(gnssTask);
}

void CRadioModule::SetupCallbacks() {
    alertTenant.Subscribe(&stateMachineUpdater);
    sntpServerTenant.Register();
    alertTenant.Register();
}
