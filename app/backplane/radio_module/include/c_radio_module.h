#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include "n_radio_module_types.h"
#include "c_gnss_tenant.h"
#include "c_udp_listener_tenant.h"
#include "c_lora_transmit_tenant.h"
#include "c_lora_to_udp_tenant.h"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_task.h>
#include <f_core/net/device/c_lora.h>

class CRadioModule : public CProjectConfiguration {
public:
    /**
     * Constructor
     */
    CRadioModule();

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
    static constexpr const char* ipAddrStr = "10.2.1.1";

    static constexpr uint16_t powerModuleTelemetryPort = 11000;
    static constexpr uint16_t radioModuleSourcePort = 12000;
    static constexpr uint16_t sensorModuleTelemetryPort = 12100;

    // Devices
    CLora lora;

    // Message Ports
    CMessagePort<NRadioModuleTypes::RadioBroadcastData>& loraBroadcastMessagePort;
    CMessagePort<NRadioModuleTypes::RadioBroadcastData>& udpBroadcastMessagePort;

    // Tenants
    CGnssTenant gnssTenant{"GNSS Tenant", &loraBroadcastMessagePort};

    CLoraTransmitTenant loraTransmitTenant{"LoRa Transmit Tenant", lora, &loraBroadcastMessagePort};
    CUdpListenerTenant sensorModuleListenerTenant{"Sensor Module Listener Tenant", ipAddrStr, sensorModuleTelemetryPort, &loraBroadcastMessagePort};
    CUdpListenerTenant powerModuleListenerTenant{"Power Module Listener Tenant", ipAddrStr, powerModuleTelemetryPort, &loraBroadcastMessagePort};

    CLoraToUdpTenant loraReceiveTenant{"LoRa Receive Tenant", lora, ipAddrStr, radioModuleSourcePort};

    // Tasks
    CTask networkingTask{"UDP Listener Task", 14, 1024, 0};
    CTask gnssTask{"GNSS Task", 15, 1024, 0};
    CTask loraTask{"LoRa Task", 15, 1024, 0};
};



#endif //C_SENSOR_MODULE_H
