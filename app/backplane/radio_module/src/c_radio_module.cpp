#ifndef CONFIG_RADIO_MODULE_RECEIVER

#include "c_radio_module.h"
#include "c_pad_state_machine_updater.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>
#include <zephyr/drivers/gnss.h>

K_MSGQ_DEFINE(loraBroadcastQueue, sizeof(NTypes::RadioBroadcastData), 10, 4);
K_MSGQ_DEFINE(udpBroadcastQueue, sizeof(NTypes::RadioBroadcastData), 10, 4);
K_MSGQ_DEFINE(gnssDataLogQueue, sizeof(NTypes::GnssLoggingData), 10, 4);
static auto loraBroadcastMsgQueue = CMsgqMessagePort<NTypes::RadioBroadcastData>(loraBroadcastQueue);
static auto udpBroadcastMsgQueue = CMsgqMessagePort<NTypes::RadioBroadcastData>(udpBroadcastQueue);
static auto gnssLogMsgQueue = CMsgqMessagePort<NTypes::GnssLoggingData>(gnssDataLogQueue);

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
    networkingTask.AddTenant(alertTenant);

#ifndef CONFIG_ARCH_POSIX
    // LoRa
    loraTask.AddTenant(loraTransmitTenant);
    loraTask.AddTenant(loraReceiveTenant);
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

void CRadioModule::SetupCallbacks() {}

#endif //CONFIG_RADIO_MODULE_RECEIVER