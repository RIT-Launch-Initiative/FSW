#include "c_power_module.h"

#include <n_autocoder_types.h>

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_zbus_message_port.h>
#include <f_core/utils/n_time_utils.h>


ZBUS_CHAN_DEFINE(sensorChannel,            // Name
                 NTypes::TimestampedSensorData, // Type
                 NULL,                          // Validator
                 NULL,                          // Observer notification callback
                 ZBUS_OBSERVERS_EMPTY,          // Initial observers
                 {0}                            // Initial value
);

static auto sensorMessagePort = CZbusMessagePort<NTypes::TimestampedSensorData>(sensorChannel);

CPowerModule::CPowerModule() : CProjectConfiguration(), sensorDataMessagePort(sensorMessagePort){}

void CPowerModule::AddTenantsToTasks() {
    // Networking
    networkTask.AddTenant(alertTenant);

    // Sensing
    sensingTask.AddTenant(sensingTenant);

    // Data Logging
    dataLoggingTask.AddTenant(dataLoggerTenant);
}

void CPowerModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkTask);

    // Sensing
    NRtos::AddTask(sensingTask);

    // Data Logging
    NRtos::AddTask(dataLoggingTask);
}

void CPowerModule::SetupCallbacks() {
    sensorDataMessagePort.("telemetry", *(new CUdpSocket(CIPv4(ipAddrStr), NNetworkDefs::POWER_MODULE_INA_DATA_PORT, NNetworkDefs::POWER_MODULE_INA_DATA_PORT)));
    udpSockets.Insert("downlink", *(new CUdpSocket(CIPv4(ipAddrStr), NNetworkDefs::POWER_MODULE_DOWNLINK_DATA_PORT, NNetworkDefs::POWER_MODULE_DOWNLINK_DATA_PORT)));

    alertTenant.Subscribe(&sensingTenant);

    // Not a callback, but ¯\_(ツ)_/¯
    // Maybe have Add and Setup tasks be private and have main.cpp call a single function?
    // Configuration children would call CBase::Setup and then can add their own setup below
    NTimeUtils::SntpSynchronize(rtc, sntpServerAddr, 5, K_MSEC(100));
}

void CPowerModule::Cleanup() {
    dataLoggerTenant.Cleanup();
}

