#ifndef C_SENSOR_MODULE_H
#define C_SENSOR_MODULE_H

#include "c_gnss_tenant.h"
#include "c_bcast_rcv_tenant.h"

// F-Core Includes
#include <c_lora_transmit_tenant.h>
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_task.h>
#include <f_core/types.h>


class CRadioModule : public CProjectConfiguration {
public:
    struct RadioBroadcastData {
        uint8_t port;
        uint8_t data[];
        uint8_t size;
    };

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
    CLora lora = DEVICE_DT_GET(DT_ALIAS(lora));

    // Message Ports
    CMessagePort<RadioBroadcastData>& loraBroadcastMessagePort;

    // Tenants
    CGnssTenant gnssTenant{"GNSS Tenant"};
    CLoraTransmitTenant loraTransmitTenant{"LoRa Transmit Tenant", lora, loraBroadcastMessagePort};
    CBroadcastReceiveTenant broadcastReceiveTenant{"Broadcast Receive Tenant", "10.1.1.1", 10000};

    // Tasks
    CTask networkTask{"Networking Task", 15, 128, 0};
    CTask gnssTask{"GNSS Task", 15, 128, 0};
    CTask loraTask{"LoRa Task", 15, 128, 0};
};



#endif //C_SENSOR_MODULE_H
