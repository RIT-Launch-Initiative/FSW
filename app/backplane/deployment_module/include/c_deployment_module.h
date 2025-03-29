#ifndef C_DEPLOYMENT_MODULE_H
#define C_DEPLOYMENT_MODULE_H

// F-Core Includes
#include <c_pyro_control_observer.h>
#include <f_core/c_project_configuration.h>
#include <f_core/os/c_task.h>
#include <f_core/os/flight_log.hpp>
#include <f_core/os/tenants/c_datalogger_tenant.h>
#include <f_core/net/application/c_udp_alert_tenant.h>
#include <n_autocoder_network_defs.h>

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
    void Cleanup() = delete;

  private:
    static std::string generateFlightLogPath();

    std::string ipAddrStr = CREATE_IP_ADDR(NNetworkDefs::DEPLOYMENT_MODULE_IP_ADDR_BASE, 1, CONFIG_MODULE_ID);

    // Tenants
    CUdpAlertTenant alertTenant{"Alert Tenant", ipAddrStr.c_str(), NNetworkDefs::ALERT_PORT};

    // Tasks
    CTask networkTask{"Networking Task", 15, 2048, 5};

    // Observers
    CPyroControlObserver pyroControlObserver;
};

#endif //C_DEPLOYMENT_MODULE_H
