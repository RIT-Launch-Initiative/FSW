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

#include <n_autocoder_network_defs.h>

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
    const char* ipAddrStr = (CREATE_IP_ADDR(NNetworkDefs::RADIO_MODULE_IP_ADDR_BASE, 1, CONFIG_MODULE_ID)).c_str();
    static constexpr uint16_t radioModuleCommandPort = NNetworkDefs::RADIO_MODULE_COMMAND_PORT;
    static constexpr uint16_t radioModuleDataRequestPort = NNetworkDefs::RADIO_MODULE_DATA_REQUEST_PORT;

    // Devices
    CLora lora;

    // Message Ports
    CMessagePort<NTypes::RadioBroadcastData>& loraBroadcastMessagePort;
    CMessagePort<NTypes::RadioBroadcastData>& udpBroadcastMessagePort;

    // Tenants
    CLoraTransmitTenant loraTransmitTenant{"LoRa Transmit Tenant", lora, &loraBroadcastMessagePort};
    CUdpListenerTenant commandListenerTenant{"Radio Module Command Listener Tenant", ipAddrStr, radioModuleCommandPort, &loraBroadcastMessagePort};
    CUdpListenerTenant dataRequestListenerTenant{"Radio Module Port Data Request Listener Tenant", ipAddrStr, radioModuleDataRequestPort, &loraBroadcastMessagePort};

    CLoraReceiveTenant loraReceiveTenant{"LoRa Receive Tenant", loraTransmitTenant, ipAddrStr, NNetworkDefs::RADIO_BASE_PORT};

    // Tasks
    CTask networkingTask{"UDP Listener Task", 15, 1024, 0};
    CTask loraTask{"LoRa Rx Task", 15, 2048, 0};
};

#endif //C_RECEIVER_MODULE_H
#endif //RADIO_MODULE_RECEIVER