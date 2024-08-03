/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/os/n_rtos.h>
#include <f_core/os/c_task.h>
#include "c_publisher.h"
#include "c_receiver.h"
#include "message.h"

#include <f_core/messaging/c_msgq_message_port.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

K_MSGQ_DEFINE(messageQueue, sizeof(Message), 10, 4);
K_MSGQ_DEFINE(completedQueue, sizeof(bool), 1, 4);

int main() {
    static CMsgqMessagePort<Message> messagePort(messageQueue);
    static CMsgqMessagePort<bool> completedPort(completedQueue);

    static CPublisher publisher(messagePort);
    static CTask publisherTask("Publisher Task", 15, 512);
    publisherTask.AddTenant(publisher);

    static CReceiver receiver(messagePort, completedPort, 10);
    static CTask receiverTask("Receiver Task", 15, 512);
    receiverTask.AddTenant(receiver);

    NRtos::AddTask(publisherTask);
    NRtos::AddTask(receiverTask);

    NRtos::StartRtos();

    bool completed = false;
    LOG_INF("Waiting for completion");
    do {
        completedPort.Receive(completed, K_FOREVER);
        if (completed == true) {
            break;
        }

        LOG_WRN("Completed flag not set. Waiting for another message.");
    } while (true);

    NRtos::StopRtos();

    return 0;
}
