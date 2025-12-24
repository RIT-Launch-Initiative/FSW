#include "c_radio_module.h"

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_msgq_message_port.h>
#include <zephyr/drivers/gnss.h>

K_MSGQ_DEFINE(loraBroadcastQueue, sizeof(LaunchLoraFrame), 10, 4);
static auto loraBroadcastMsgQueue = CMsgqMessagePort<LaunchLoraFrame>(loraBroadcastQueue);

K_MSGQ_DEFINE(gnssDataLogQueue, sizeof(NTypes::GnssData), 10, 4);
static auto gnssLogMsgQueue = CMsgqMessagePort<NTypes::GnssData>(gnssDataLogQueue);

CRadioModule::CRadioModule() : CProjectConfiguration(),
#ifndef CONFIG_ARCH_POSIX
                               lora(*DEVICE_DT_GET(DT_ALIAS(lora))),
#endif
                               loraDownlinkMessagePort(loraBroadcastMsgQueue),
                                gnssDataLogMessagePort(gnssLogMsgQueue) {}

void CRadioModule::AddTenantsToTasks() {
    // Networking
    networkingTask.AddTenant(sensorModuleListenerTenant);
    networkingTask.AddTenant(powerModuleListenerTenant);

#ifndef CONFIG_ARCH_POSIX
    // LoRa
    loraTenant.RegisterFrameHandler(NNetworkDefs::RADIO_MODULE_COMMAND_PORT, remoteGpioHandler);
    loraTenant.RegisterFrameHandler(NNetworkDefs::RADIO_MODULE_DATA_REQUEST_PORT, downlinkSchedulerTenant);
    loraTenant.RegisterDefaultFrameHandler(loraToUdpHandler);

    loraTask.AddTenant(downlinkSchedulerTenant);
    loraTask.AddTenant(loraTenant);

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
