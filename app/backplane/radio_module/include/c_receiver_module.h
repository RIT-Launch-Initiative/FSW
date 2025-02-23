#ifdef CONFIG_RADIO_MODULE_RECEIVER
#ifndef C_RECEIVER_MODULE_H
#define C_RECEIVER_MODULE_H

#include "n_radio_module_types.h"
#include "c_gnss_tenant.h"
#include "c_lora_receive_tenant.h"
#include "c_lora_transmit_tenant.h"
#include "c_udp_listener_tenant.h"

// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_task.h>
#include <f_core/radio/c_lora.h>

class CReceiverModule : public CProjectConfiguration {
public:
    /**
     * Constructor
     */
    CReceiverModule();

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
    static constexpr uint16_t radioModuleSourcePort = 12000;
    static constexpr uint16_t radioModuleCommandSourcePort = 12001;

    // Devices
    CLora lora;

    // Message Ports
    CMessagePort<NTypes::RadioBroadcastData>& loraBroadcastMessagePort;
    CMessagePort<NTypes::RadioBroadcastData>& udpBroadcastMessagePort;

    // Tenants
    CLoraTransmitTenant loraTransmitTenant{"LoRa Transmit Tenant", lora, &loraBroadcastMessagePort};
    CUdpListenerTenant commandListenerTenant{"Radio Module Command Listener Tenant", ipAddrStr, radioModuleCommandSourcePort, &loraBroadcastMessagePort};

    CLoraReceiveTenant loraReceiveTenant{"LoRa Receive Tenant", lora, ipAddrStr, radioModuleSourcePort, &loraBroadcastMessagePort};

    // Tasks
    CTask networkingTask{"UDP Listener Task", 15, 1024, 0};
    CTask loraTask{"LoRa Rx Task", 15, 2048, 0};
};

#endif //C_RECEIVER_MODULE_H
#endif //RADIO_MODULE_RECEIVER