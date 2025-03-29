#ifndef C_DEPLOYMENT_MODULE_H
#define C_DEPLOYMENT_MODULE_H

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/os/c_task.h>
#include <f_core/os/flight_log.hpp>
#include <f_core/os/tenants/c_datalogger_tenant.h>
#include <n_autocoder_network_defs.h>
#include <f_core/device/c_rtc.h>

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
    void Cleanup() override;

  private:
    static std::string generateFlightLogPath();

    // Devices
    CRtc rtc{*DEVICE_DT_GET(DT_ALIAS(rtc))};

    std::string ipAddrStr = CREATE_IP_ADDR(NNetworkDefs::DEPLOYMENT_MODULE_IP_ADDR_BASE, 1, CONFIG_MODULE_ID);
    const char* sntpServerAddr = "10.2.1.1"; // TODO: Maybe we should look into hostnames? Also, still need to fix the create ip addr bug...

    CFlightLog flight_log;

    // Tenants
    // TODO: Tenant for listening to events

    // Tasks
    CTask networkTask{"Networking Task", 15, 1024, 0};
};

#endif //C_DEPLOYMENT_MODULE_H
