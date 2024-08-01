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



int main() {
    CMsgqMessagePort<Message> messagePort(10);

    static CPublisher publisher(messagePort);
    static CReceiver receiver(messagePort);
    static CTask printTask("Print Task", 15, 512, 1000);

    printTask.AddTenant(printWorldTenant);
    NRtos::AddTask(printTask);

    int count = 0;
    static CPublisher counterTenantOne("Counter 1", &count);
    static CTask counterTask("Counter Task", 15, 512, 80);

    counterTask.AddTenant(counterTenantOne);
    counterTask.AddTenant(counterTenantTwo);
    NRtos::AddTask(counterTask);

    NRtos::StartRtos();
    NRtos::StopRtos();

    return 0;
}
