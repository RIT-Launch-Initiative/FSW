#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include "c_sensing_tenant.h"
#include "flight.hpp"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/n_alerts.h>
#include <f_core/net/application/c_udp_broadcast_tenant.h>
#include <f_core/os/c_task.h>
#include <f_core/os/flight_log.hpp>
#include <f_core/os/tenants/c_datalogger_tenant.h>
#include <n_autocoder_network_defs.h>
#include <n_autocoder_types.h>
#include <f_core/device/c_rtc.h>

class CSensorModule : public CProjectConfiguration {
  public:
    /**
     * Constructor
     */
    CSensorModule();

    /**
     * See parent docs
     */
    void AddTenantsToTasks() override;

    /**
     * See parent docs
     */
    void AddTasksToRtos() override;

    /**
     * See parent docs
     */
    void SetupCallbacks() override;

    /**
    * Cleanup
    */
    void Cleanup() { dataLoggerTenant.Cleanup(); }

  private:
    static std::string generateFlightLogPath();

    std::string ipAddrStr = CREATE_IP_ADDR(NNetworkDefs::SENSOR_MODULE_IP_ADDR_BASE, 1, CONFIG_MODULE_ID);
    const char* sntpServerAddr = "10.2.1.1"; // TODO: Maybe we should look into hostnames? Also, still need to fix the create ip addr bug...

    static constexpr int telemetryBroadcastPort = NNetworkDefs::SENSOR_MODULE_TELEMETRY_PORT;
    static constexpr int alertPort = NNetworkDefs::ALERT_PORT;

    // Devices
    CRtc rtc{*DEVICE_DT_GET(DT_ALIAS(rtc))};

    // Message Ports
    CMessagePort<NTypes::SensorData>& sensorDataBroadcastMessagePort;
    CMessagePort<NTypes::SensorData>& sensorDataLogMessagePort;
    CMessagePort<std::array<uint8_t, 7>>& alertMessagePort;

    CFlightLog flight_log;
    SensorModulePhaseController controller{sourceNames, eventNames, timer_events, deciders, &flight_log};
    CDetectionHandler detectionHandler{controller, alertMessagePort};

    // Tenants
    CSensingTenant sensingTenant{"Sensing Tenant", sensorDataBroadcastMessagePort, sensorDataLogMessagePort,
                             detectionHandler};
    CUdpBroadcastTenant<NTypes::SensorData> broadcastTenant{"Broadcast Tenant", ipAddrStr.c_str(), telemetryBroadcastPort, telemetryBroadcastPort, sensorDataBroadcastMessagePort};
    CUdpBroadcastTenant<std::array<uint8_t, 7>> udpAlertTenant{"UDP Alert Tenant", ipAddrStr.c_str(), alertPort, alertPort, alertMessagePort};
    CDataLoggerTenant<NTypes::SensorData> dataLoggerTenant{"Data Logger Tenant", "/lfs/sensor_module_data.bin", LogMode::Growing, 0, sensorDataLogMessagePort};

    // Tasks
    CTask networkTask{"Networking Task", 15, 3072, 5};
    CTask sensingTask{"Sensing Task", 14, 1024, 10};
    CTask dataLogTask{"Data Logging Task", 15, 1300, 5};
};

#endif //C_SENSOR_MODULE_H
