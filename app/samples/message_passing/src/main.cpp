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

#include <iso646.h>
#include <f_core/messaging/c_msgq_message_port.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

int main() {
    CMsgqMessagePort<Message> messagePort(10);
    CMsgqMessagePort<bool> completedPort(1);

    static CPublisher publisher(messagePort);
    static CTask publisherTask("Publisher Task", 15, 512, 1000);
    publisherTask.AddTenant(publisher);

    static CReceiver receiver(messagePort, completedPort, 10);
    static CTask receiverTask("Receiver Task", 15, 512, 1000);
    receiverTask.AddTenant(receiver);

    NRtos::AddTask(publisherTask);
    NRtos::AddTask(receiverTask);

    NRtos::StartRtos();

    bool completed = false;
    do {
        completedPort.Receive(&completed, K_FOREVER);
        if (completed) {
            break;
        }

        LOG_WRN("Completed flag not set. Waiting for another message.");
    } while (true);

    NRtos::StopRtos();

    return 0;
}
