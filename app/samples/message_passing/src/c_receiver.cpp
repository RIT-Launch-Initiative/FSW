#include <f_core/os/c_tenant.h>
#include "c_receiver.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CReceiver);

CReceiver::CReceiver(CMessagePort<Message>& messagePort, CMessagePort<bool>& completedPort, int messageCountToReceive) :
    CTenant("Receiver"), messagePort(messagePort), completedPort(completedPort),
    messageCountToReceive(messageCountToReceive) {

}

void CReceiver::Startup() {
    CBase::Startup(); // Initialize any parent functionality
}

void CReceiver::Run() {
    if (messageCountToReceive == 0) {
        completedPort.Send(true);
        return;
    }

    Message message{};
    messagePort.Receive(&message);
    LOG_INF("%d: %s", message.count, message.message);

    messageCountToReceive--;
}

