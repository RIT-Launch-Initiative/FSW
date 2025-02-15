#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include "c_sensing_tenant.h"
#include "c_adc_tenant.h"

#include <n_autocoder_types.h>

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_task.h>
#include <f_core/os/tenants/c_datalogger_tenant.h>
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
    static constexpr int telemetryBroadcastPort = NNetworkDefs::SENSOR_MODULE_TELEMETRY_PORT;

    // Message Ports
    CMessagePort<NTypes::SensorData>& sensorDataBroadcastMessagePort;
    CMessagePort<NTypes::SensorData>& sensorDataLogMessagePort;
    CMessagePort<int32_t>& adcDataBroadcastMessagePort;
    CMessagePort<int32_t>& adcDataLogMessagePort;

    // Tenants
    CSensingTenant sensingTenant{"Sensing Tenant", sensorDataBroadcastMessagePort, sensorDataLogMessagePort};
    CUdpBroadcastTenant<NTypes::SensorData> sensorBroadcastTenant{"Sensor Broadcast Tenant", ipAddrStr, telemetryBroadcastPort, telemetryBroadcastPort, sensorDataBroadcastMessagePort};
    CDataLoggerTenant<NTypes::SensorData> dataLoggerTenant{"Data Logger Tenant", "/lfs/sensor_data.bin", LogMode::Growing, 0, sensorDataLogMessagePort};
    CAdcTenant adcTenant{"ADC Tenant", adcDataBroadcastMessagePort, adcDataLogMessagePort};
    CUdpBroadcastTenant<int32_t> adcBroadcastTenant{"ADC Broadcast Tenant", ipAddrStr, telemetryBroadcastPort, telemetryBroadcastPort, adcDataBroadcastMessagePort};

    // Tasks
    CTask networkTask{"Networking Task", 15, 1024, 0};
    CTask sensingTask{"Sensing Task", 15, 1024, 0};
    CTask dataLoggingTask{"Data Logging Task", 15, 1500, 0};
};

#endif //C_SENSOR_MODULE_H
