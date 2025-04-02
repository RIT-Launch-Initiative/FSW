#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include "c_sensing_tenant.h"

#include <n_autocoder_types.h>

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/device/c_rtc.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_task.h>
#include <f_core/os/tenants/c_datalogger_tenant.h>
#include <f_core/net/application/c_udp_alert_tenant.h>
#include <f_core/net/application/c_udp_broadcast_tenant.h>

#include <n_autocoder_network_defs.h>

class CPowerModule : public CProjectConfiguration {
public:
    /**
     * Constructor
     */
    CPowerModule();

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
     * See parent docs
     */
    void Cleanup();

private:
    const char* ipAddrStr = (CREATE_IP_ADDR(NNetworkDefs::POWER_MODULE_IP_ADDR_BASE, 2, CONFIG_MODULE_ID)).c_str();
    const char* sntpServerAddr = "10.2.1.1"; // TODO: Maybe we should look into hostnames? Also, still need to fix the create ip addr bug...
    static constexpr int telemetryBroadcastPort = NNetworkDefs::POWER_MODULE_INA_DATA_PORT;

    // Devices
    CRtc rtc{*DEVICE_DT_GET(DT_ALIAS(rtc))};

    // Message Ports
    CMessagePort<NTypes::SensorData>& sensorDataBroadcastMessagePort;
    CMessagePort<NTypes::SensorData>& sensorDataLogMessagePort;

    // Tenants
    CSensingTenant sensingTenant{"Sensing Tenant", sensorDataBroadcastMessagePort, sensorDataLogMessagePort};
    CUdpBroadcastTenant<NTypes::SensorData> broadcastTenant{"Broadcast Tenant", ipAddrStr, telemetryBroadcastPort, telemetryBroadcastPort, sensorDataBroadcastMessagePort};
    CDataLoggerTenant<NTypes::SensorData> dataLoggerTenant{"Data Logger Tenant", "/lfs/sensor_data.bin", LogMode::Growing, 0, sensorDataLogMessagePort};
    CUdpAlertTenant alertTenant{"Alert Tenant", ipAddrStr, NNetworkDefs::ALERT_PORT};

    // Tasks
    CTask networkTask{"Networking Task", 15, 3072, 0};
    CTask sensingTask{"Sensing Task", 15, 1024, 0};
    CTask dataLoggingTask{"Data Logging Task", 15, 1500, 0};
};



#endif //C_SENSOR_MODULE_H
