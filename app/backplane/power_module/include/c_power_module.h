#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include "c_sensing_tenant.h"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_task.h>
#include <f_core/os/tenants/c_datalogger_tenant.h>
#include <f_core/types.h>
#include <f_core/net/application/c_udp_broadcast_tenant.h>


class CPowerModule : public CProjectConfiguration {
public:

    // TODO(aaron) Break this apart based on telemetry frequency eventually
    struct __attribute__((packed)) SensorData {
      NTypes::NSensor::ShuntData RailBattery;
      NTypes::NSensor::ShuntData Rail3v3;
      NTypes::NSensor::ShuntData Rail5v0;
    };

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

private:
    static constexpr const char* ipAddrStr = "10.1.2.1";
    static constexpr int telemetryBroadcastPort = 11000;

    // Message Ports
    CMessagePort<SensorData>& sensorDataBroadcastMessagePort;

    // Tenants
    CSensingTenant sensingTenant{"Sensing Tenant"};
    CUdpBroadcastTenant<SensorData> broadcastTenant{"Broadcast Tenant", ipAddrStr, telemetryBroadcastPort, telemetryBroadcastPort, sensorDataBroadcastMessagePort};
    // CDataLoggerTenant<SensorData> dataLoggerTenant{"Data Logger Tenant", "/lfs/sensor_data.bin", LogMode::Growing, 0, sensorDataBroadcastMessagePort};

    // Tasks
    CTask networkTask{"Networking Task", 15, 512, 0};
    CTask sensingTask{"Sensing Task", 15, 1024, 0};
    CTask dataLoggingTask{"Data Logging Task", 15, 512, 0};
};



#endif //C_SENSOR_MODULE_H
