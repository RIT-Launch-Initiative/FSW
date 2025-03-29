#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include "c_sensing_tenant.h"
#include "flight.hpp"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/n_alerts.h>
#include <f_core/net/application/c_udp_broadcast_tenant.h>
#include <f_core/net/application/c_tftp_server_tenant.h>
#include <f_core/os/c_task.h>
#include <f_core/os/flight_log.hpp>
#include <f_core/os/tenants/c_datalogger_tenant.h>
#include <n_autocoder_network_defs.h>
#include <n_autocoder_types.h>

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
    static constexpr int telemetryBroadcastPort = NNetworkDefs::SENSOR_MODULE_TELEMETRY_PORT;
    static constexpr int alertPort = NNetworkDefs::ALERT_PORT;

    // Message Ports
    CMessagePort<NTypes::SensorData>& sensorDataBroadcastMessagePort;
    CMessagePort<NTypes::SensorData>& sensorDataLogMessagePort;
    CMessagePort<NAlerts::AlertType>& alertMessagePort;

    CFlightLog flight_log;
    SensorModulePhaseController controller{sourceNames, eventNames, timer_events, deciders, &flight_log};
    CDetectionHandler detectionHandler{controller, alertMessagePort};

    // Tenants
    CSensingTenant sensingTenant{"Sensing Tenant", sensorDataBroadcastMessagePort, sensorDataLogMessagePort,
                             detectionHandler};
    CUdpBroadcastTenant<NTypes::SensorData> broadcastTenant{"Broadcast Tenant", ipAddrStr.c_str(), telemetryBroadcastPort, telemetryBroadcastPort, sensorDataBroadcastMessagePort};
    CUdpBroadcastTenant<NAlerts::AlertType> udpAlertTenant{"UDP Alert Tenant", ipAddrStr.c_str(), alertPort, alertPort, alertMessagePort};
    CDataLoggerTenant<NTypes::SensorData> dataLoggerTenant{"Data Logger Tenant", "/lfs/sensor_module_data.bin", LogMode::Growing, 0, sensorDataLogMessagePort};
    CTftpServerTenant tftpServerTenant = *CTftpServerTenant::getInstance(CIPv4(ipAddrStr.c_str()));


    // Tasks
    CTask networkTask{"Networking Task", 15, 3072, 5};
    CTask sensingTask{"Sensing Task", 14, 8092, 50};
    CTask dataLogTask{"Data Logging Task", 15, 4096, 5};
};

#endif //C_SENSOR_MODULE_H
