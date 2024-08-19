#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include "c_sensing_tenant.h"
#include "c_broadcast_tenant.h"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_task.h>
#include <f_core/types.h>


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
    // Message Ports
    CMessagePort<SensorData>& sensorDataBroadcastMessagePort;

    // Tenants
    CSensingTenant sensingTenant{"Sensing Tenant"};
    CBroadcastTenant broadcastTenant{"Broadcast Tenant", "10.0.0.0", 12000, 12000};

    // Tasks
    CTask networkTask{"Networking Task", 15, 128, 0};
    CTask sensingTask{"Sensing Task", 15, 128, 0};
};



#endif //C_SENSOR_MODULE_H