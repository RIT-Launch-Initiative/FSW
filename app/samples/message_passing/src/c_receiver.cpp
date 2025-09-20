#include <f_core/os/c_runnable_tenant.h>
#include "c_receiver.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Receiver);

CReceiver::CReceiver(CMessagePort<Message>& messagePort, CMessagePort<bool>& completedPort, int messageCountToReceive) :
    CTenant("Receiver"), messagePort(messagePort), completedPort(completedPort),
    messageCountToReceive(messageCountToReceive) {

}

void CReceiver::Run() {
    if (messageCountToReceive == 0) {
        completedPort.Send(true);
        return;
    }

    Message message{};
    messagePort.Receive(message, K_NO_WAIT);
    LOG_INF("%d - %s", message.count, message.message);

    messageCountToReceive--;
}

