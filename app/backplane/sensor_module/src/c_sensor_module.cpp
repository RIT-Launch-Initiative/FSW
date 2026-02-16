#include "c_sensor_module.h"

// F-Core Tenant
#include <f_core/messaging/c_msgq_message_port.h>
#include <f_core/messaging/c_latest_message_port.h>
#include <f_core/os/n_rtos.h>
#include <f_core/os/tenants/c_timed_tenant.h> // TODO: Blast once we have CCpuMonitorTenant inherit this
#include <f_core/utils/n_time_utils.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sensor_module);

static auto broadcastMsgQueue = CLatestMessagePort<NTypes::SensorData>();
static auto downlinkMsgQueue = CLatestMessagePort<NTypes::LoRaBroadcastSensorData>();

K_MSGQ_DEFINE(dataLogQueue, sizeof(NTypes::TimestampedSensorData), 12, 4);
static auto dataLogMsgQueue = CMsgqMessagePort<NTypes::TimestampedSensorData>(dataLogQueue);

K_MSGQ_DEFINE(alertQueue, sizeof(NAlerts::AlertPacket), 4, 4);
static auto alertMsgQueue = CMsgqMessagePort<NAlerts::AlertPacket>(alertQueue);

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
