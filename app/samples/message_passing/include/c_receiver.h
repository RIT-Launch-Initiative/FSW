#ifndef HELLOTENANT_H
#define HELLOTENANT_H

#include "message.h"

// F-Core Includes
#include <f_core/messaging/c_message_port.h>
#include <f_core/os/c_tenant.h>

class CReceiver : public CTenant {
public:
    /**
     * Constructor
     * @param messagePort port to receive data from
     * @param completedPort port to receive data from
     * @param messageCountToReceive The number of messages to receive before stopping the RTOS
     */
    explicit CReceiver(CMessagePort<Message> &messagePort, CMessagePort<bool> &completedPort, int messageCountToReceive);

    /**
     * See parent docs
     */
    void Run() override;

private:
    CMessagePort<Message> &messagePort;
    CMessagePort<bool> &completedPort;

    const int messageCountToReceive;
};

#endif //HELLOTENANT_H
