#ifndef C_RADIO_TRANSMITTER_H
#define C_RADIO_TRANSMITTER_H

#include "c_radio_module.h"


// F-Core Includes
#include <f_core/c_project_configuration.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_task.h>
#include <f_core/types.h>
#include <f_core/net/application/c_udp_broadcast_tenant.h>

class CRadioReceiver : public CRadioModule {
public:
    /**
     * Constructor
     */
    CRadioReceiver();

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
    using CBase = CRadioModule;

};



#endif //C_RADIO_TRANSMITTER_H
