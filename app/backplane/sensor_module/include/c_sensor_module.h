#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include "c_sensing_tenant.h"
#include "c_rs485_tenant.h"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_task.h>
#include <f_core/types.h>
#include <f_core/net/application/c_udp_broadcast_tenant.h>


class CSensorModule : public CProjectConfiguration {
public:

    // TODO(aaron) Break this apart based on telemetry frequency eventually
    struct __attribute__((packed)) SensorData {
      NTypes::NSensor::BarometerData PrimaryBarometer;
      NTypes::NSensor::BarometerData SecondaryBarometer;

      NTypes::NSensor::AccelerometerData Acceleration;

      NTypes::NSensor::AccelerometerData ImuAcceleration;
      NTypes::NSensor::GyroscopeData ImuGyroscope;

      NTypes::NSensor::MagnetometerData Magnetometer;

      NTypes::NSensor::TemperatureData Temperature;
    };

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

private:
    static constexpr const char* ipAddrStr = "10.3.2.1";
    static constexpr int telemetryBroadcastPort = 12100;

    // Message Ports
    CMessagePort<SensorData>& sensorDataBroadcastMessagePort;

    // Tenants
    CSensingTenant sensingTenant{"Sensing Tenant"};
    // CRs485Tenant rs485Tenant{"RS485 Tenant"};
    CUdpBroadcastTenant<SensorData> broadcastTenant{"Broadcast Tenant", ipAddrStr, telemetryBroadcastPort, telemetryBroadcastPort, sensorDataBroadcastMessagePort};

    // Tasks
    CTask networkTask{"Networking Task", 15, 512, 0};
    CTask sensingTask{"Sensing Task", 15, 1024, 0};
};



#endif //C_SENSOR_MODULE_H
