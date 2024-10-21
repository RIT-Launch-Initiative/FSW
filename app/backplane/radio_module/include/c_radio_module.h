#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include "n_radio_module_types.h"
#include "c_gnss_tenant.h"
#include "c_udp_listener_tenant.h"
#include "c_lora_transmit_tenant.h"

// F-Core Includes
#include <c_lora_receive_tenant.h>
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
    // Devices
    CLora lora;

    // Message Ports
     CMessagePort<NRadioModuleTypes::RadioBroadcastData>& loraBroadcastMessagePort;
     CMessagePort<NRadioModuleTypes::RadioBroadcastData>& udpBroadcastMessagePort;

    // Tenants
    CGnssTenant gnssTenant{"GNSS Tenant"};

    CLoraTransmitTenant loraTransmitTenant{"LoRa Transmit Tenant", lora, &loraBroadcastMessagePort};
    CUdpListenerTenant sensorModuleListenerTenant{"Sensor Module Listener Tenant", "10.2.1.1", 11000, &loraBroadcastMessagePort};
    CUdpListenerTenant powerModuleListenerTenant{"Power Module Listener Tenant", "10.2.1.1", 13000, &loraBroadcastMessagePort};

    CLoraReceiveTenant loraReceiveTenant{"LoRa Receive Tenant", lora, &udpBroadcastMessagePort};

    // Tasks
    CTask networkingTask{"UDP Listener Task", 15, 128, 0};
    CTask gnssTask{"GNSS Task", 15, 128, 0};
    CTask loraTask{"LoRa Task", 15, 128, 0};
};



#endif //C_SENSOR_MODULE_H
