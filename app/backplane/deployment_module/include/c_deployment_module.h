#pragma once

// F-Core Includes
#include <c_pyro_control_observer.h>
#include <f_core/c_project_configuration.h>
#include <f_core/os/c_task.h>
#include <f_core/os/flight_log.hpp>
#include <f_core/os/tenants/c_datalogger_tenant.h>
#include <f_core/net/application/c_udp_alert_tenant.h>
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

  private:
    static std::string generateFlightLogPath();

    // Devices
    CRtc rtc{*DEVICE_DT_GET(DT_ALIAS(rtc))};

    std::string ipAddrStr = "10.4.1.1";
    const char* sntpServerAddr = "10.2.1.1"; // TODO: Maybe we should look into hostnames? Also, still need to fix the create ip addr bug...

    // Tenants
    CUdpAlertTenant alertTenant{"Alert Tenant", ipAddrStr.c_str(), NNetworkDefs::ALERT_PORT};

    // Tasks
    CTask networkTask{"Networking Task", 15, 2048, 0};

    // Observers
    CPyroControlObserver pyroControlObserver;
};


