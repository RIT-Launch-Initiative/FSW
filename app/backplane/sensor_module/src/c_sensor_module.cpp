#include "c_sensor_module.h"

// F-Core Tenant
#include "f_core/messaging/c_zbus_message_port.h"
#include "zephyr/zbus/zbus.h"

#include <f_core/messaging/c_msgq_message_port.h>
#include <f_core/os/n_rtos.h>
#include <f_core/utils/n_time_utils.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensor_module);

ZBUS_CHAN_DEFINE(broadcastChannel,     // Name
                 NTypes::SensorData,   // Type
                 NULL,                 // Validator
                 NULL,                 // Observer notification callback
                 ZBUS_OBSERVERS_EMPTY, // Initial observers
                 {0}                   // Initial value
);
ZBUS_CHAN_DEFINE(timestampedChannel,            // Name
                 NTypes::TimestampedSensorData, // Type
                 NULL,                          // Validator
                 NULL,                          // Observer notification callback
                 ZBUS_OBSERVERS_EMPTY,          // Initial observers
                 {0}                            // Initial value
);
ZBUS_CHAN_DEFINE(downlinkChannel,                 // Name
                 NTypes::LoRaBroadcastSensorData, // Type
                 NULL,                            // Validator
                 NULL,                            // Observer notification callback
                 ZBUS_OBSERVERS_EMPTY,            // Initial observers
                 {0}                              // Initial value
);
ZBUS_CHAN_DEFINE(alertChannel,                    // Name
                 NAlerts::AlertPacket, // Type
                 NULL,                            // Validator
                 NULL,                            // Observer notification callback
                 ZBUS_OBSERVERS_EMPTY,            // Initial observers
                 {0}                              // Initial value
);
static auto broadcastMsgQueue = CZbusMessagePort<NTypes::SensorData>(broadcastChannel);
static auto dataLogMsgQueue = CZbusMessagePort<NTypes::TimestampedSensorData>(timestampedChannel);
static auto downlinkMsgQueue = CZbusMessagePort<NTypes::LoRaBroadcastSensorData>(downlinkChannel);
static auto alertMsgQueue = CZbusMessagePort<NTypes::LoRaBroadcastSensorData>(alertChannel);

CSensorModule::CSensorModule()
    : CProjectConfiguration(), sensorDataBroadcastMessagePort(broadcastMsgQueue), downlinkMessagePort(downlinkMsgQueue),
      sensorDataLogMessagePort(dataLogMsgQueue), alertMessagePort(alertMsgQueue), flight_log{"/lfs/flight_log.txt"} {}

void CSensorModule::AddTenantsToTasks() {
    // Networking
    networkTask.AddTenant(broadcastTenant);
    networkTask.AddTenant(downlinkTelemTenant);
    networkTask.AddTenant(udpAlertTenant);

    // Sensing
    sensingTask.AddTenant(sensingTenant);

    // Data Logging
    dataLogTask.AddTenant(dataLoggerTenant);
}

void CSensorModule::AddTasksToRtos() {
    // Networking
    NRtos::AddTask(networkTask);

    // Sensing
    NRtos::AddTask(sensingTask);

    // Data Logging
    NRtos::AddTask(dataLogTask);
}

void CSensorModule::SetupCallbacks() {
    NTimeUtils::SntpSynchronize(rtc, sntpServerAddr, 5, K_MSEC(100));
}
