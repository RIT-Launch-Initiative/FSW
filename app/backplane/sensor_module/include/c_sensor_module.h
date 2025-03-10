#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include "c_sensing_tenant.h"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_task.h>
#include <f_core/os/tenants/c_datalogger_tenant.h>
#include <f_core/net/application/c_udp_broadcast_tenant.h>
#include <f_core/net/application/c_tftp_server_tenant.h>
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
    void Cleanup() {
        dataLoggerTenant.Cleanup();
    }

private:
    const char* ipAddrStr = (CREATE_IP_ADDR(NNetworkDefs::SENSOR_MODULE_IP_ADDR_BASE, 1, CONFIG_MODULE_ID).c_str()).c_str();
    static constexpr int telemetryBroadcastPort = NNetworkDefs::SENSOR_MODULE_TELEMETRY_PORT;

    // Message Ports
    CMessagePort<NTypes::SensorData>& sensorDataBroadcastMessagePort;
    CMessagePort<NTypes::SensorData>& sensorDataLogMessagePort;

    // Tenants
    CSensingTenant sensingTenant{"Sensing Tenant", sensorDataBroadcastMessagePort, sensorDataLogMessagePort};
    CUdpBroadcastTenant<NTypes::SensorData> broadcastTenant{"Broadcast Tenant", ipAddrStr, telemetryBroadcastPort, telemetryBroadcastPort, sensorDataBroadcastMessagePort};
    CDataLoggerTenant<NTypes::SensorData> dataLoggerTenant{"Data Logger Tenant", "/lfs/sensor_module_data.bin", LogMode::Growing, 0, sensorDataLogMessagePort};
    CTftpServerTenant tftpServerTenant = *CTftpServerTenant::getInstance(CIPv4(ipAddrStr));


    // Tasks
    CTask networkTask{"Networking Task", 15, 3072, 0};
    CTask sensingTask{"Sensing Task", 15, 1024, 0};
    CTask dataLogTask{"Data Logging Task", 15, 1300, 0};
};



#endif //C_SENSOR_MODULE_H
