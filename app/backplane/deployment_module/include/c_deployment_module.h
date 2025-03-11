#ifndef C_DEPLOYMENT_MODULE_H
#define C_DEPLOYMENT_MODULE_H

#include "c_sensing_tenant.h"
#include "flight.hpp"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_task.h>
#include <f_core/os/flight_log.hpp>
#include <f_core/os/tenants/c_datalogger_tenant.h>
#include <n_autocoder_network_defs.h>
#include <n_autocoder_types.h>

class CDeploymentModule : public CProjectConfiguration {
  public:
    /**
     * Constructor
     */
    CDeploymentModule();

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

    std::string ipAddrStr = CREATE_IP_ADDR(NNetworkDefs::DEPLOYMENT_MODULE_IP_ADDR_BASE, 1, CONFIG_MODULE_ID);

    CFlightLog flight_log;
    DeploymentModulePhaseController controller{sourceNames, eventNames, timer_events, deciders, &flight_log};
    CDetectionHandler detectionHandler{controller};

    // Tenants
    CDataLoggerTenant<NTypes::SensorData> dataLoggerTenant{"Data Logger Tenant", "/lfs/deployment_module_data.bin",
                                                           LogMode::Growing, 0, sensorDataLogMessagePort};

    // Tasks
    CTask networkTask{"Networking Task", 15, 1024, 0};
    CTask dataLogTask{"Data Logging Task", 15, 1300, 0};
};

#endif //C_DEPLOYMENT_MODULE_H
