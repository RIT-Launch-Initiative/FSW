#include "c_power_module.h"
#include "udp_transmit.h"

#include <n_autocoder_types.h>

// F-Core Tenant
#include <f_core/os/n_rtos.h>
#include <f_core/messaging/c_zbus_message_port.h>
#include <f_core/utils/n_time_utils.h>

LOG_MODULE_REGISTER(CPowerModule);

static CHashMap<std::string, void*> sensorChannelUserData;
ZBUS_CHAN_DEFINE(sensorChannel,                 // Name
                 NTypes::TimestampedSensorData, // Type
                 NULL,                          // Validator
                 &sensorChannelUserData,        // User data
                 ZBUS_OBSERVERS_EMPTY,          // Initial observers
                 {0}                            // Initial value
    );

ZBUS_LISTENER_DEFINE(rawDataListener, transmitRawData);
ZBUS_LISTENER_DEFINE(downlinkListener, transmitDownlinkData);


static auto sensorMessagePort = CZbusMessagePort<NTypes::TimestampedSensorData>(sensorChannel);

CPowerModule::CPowerModule() : CProjectConfiguration(), sensorDataMessagePort(sensorMessagePort) {}

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
    if (zbus_chan_add_obs(&sensorChannel, &rawDataListener, K_MSEC(100)) != 0) {
        LOG_ERR("Failed to add raw data listener to sensor channel");
    } else {
        sensorMessagePort.AddUserData("TelemetrySocket",
                                      new CUdpSocket(CIPv4(ipAddrStr.c_str()), NNetworkDefs::POWER_MODULE_INA_DATA_PORT,
                                                     NNetworkDefs::POWER_MODULE_INA_DATA_PORT));
    }

    if (zbus_chan_add_obs(&sensorChannel, &downlinkListener, K_MSEC(100)) != 0) {
        LOG_ERR("Failed to add downlink listener to sensor channel");
    } else {
        sensorMessagePort.AddUserData("DownlinkSocket",
                                      new CUdpSocket(CIPv4(ipAddrStr.c_str()),
                                                     NNetworkDefs::POWER_MODULE_DOWNLINK_DATA_PORT,
                                                     NNetworkDefs::POWER_MODULE_DOWNLINK_DATA_PORT));
    }

    alertTenant.Subscribe(&sensingTenant);

    // Not a callback, but ¯\_(ツ)_/¯
    // Maybe have Add and Setup tasks be private and have main.cpp call a single function?
    // Configuration children would call CBase::Setup and then can add their own setup below
    NTimeUtils::SntpSynchronize(rtc, sntpServerAddr, 5, K_MSEC(100));
}

void CPowerModule::Cleanup() {
    dataLoggerTenant.Cleanup();
}

