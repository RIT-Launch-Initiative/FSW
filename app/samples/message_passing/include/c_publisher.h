#pragma once

#include "message.h"

// F-Core Includes
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_runnable_tenant.h>

/**
 * Increments and prints the current count for a given integer.
 */
class CPublisher : public CRunnableTenant {
public:
    /**
     * Constructor.
     * @param name The name of the tenant.
     * @param count The integer to increment and print.
     */
    explicit CPublisher(CMessagePort<Message> &messagePort);

    /**
     * See parent docs
     */
    void Startup() override;

    /**
     * See parent docs
     */
    void Run() override;

private:
    using CBase = CRunnableTenant;

    CMessagePort<Message> &messagePort;
    Message message;
};

