#include "c_publisher.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Publisher);

CPublisher::CPublisher(CMessagePort<Message> &messagePort) : CTenant("Publisher"), messagePort(messagePort), message({})  {
}

void CPublisher::Startup() {
    CBase::Startup(); // Initialize any parent functionality

    message.count = 0;
    strncpy(message.message, "Hello, World!", sizeof(message.message));
}

void CPublisher::Run() {
    message.count++;
    messagePort.Send(message, K_NO_WAIT);
}


