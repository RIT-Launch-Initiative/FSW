#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include "n_radio_module_types.h"
#include "c_gnss_tenant.h"
#include "c_udp_listener_tenant.h"
#include "c_lora_transmit_tenant.h"

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
    // Devices
    CLora lora;

    // Message Ports
     CMessagePort<NRadioModuleTypes::RadioBroadcastData>& loraBroadcastMessagePort;

    // Tenants
    CGnssTenant gnssTenant{"GNSS Tenant"};
    CLoraTransmitTenant loraTransmitTenant{"LoRa Transmit Tenant", lora, &loraBroadcastMessagePort};

    // Listener Tenants
    CUdpListenerTenant udpListenerTenant{"Broadcast Receive Tenant", "10.1.1.1", 10000, &loraBroadcastMessagePort};

    // Tasks
    CTask udpListenerTask{"UDP Listener Task", 15, 128, 0};
    CTask gnssTask{"GNSS Task", 15, 128, 0};
    CTask loraTask{"LoRa Task", 15, 128, 0};
};



#endif //C_SENSOR_MODULE_H
