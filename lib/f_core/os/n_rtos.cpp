/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <f_core/os/n_rtos.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(NRtos);

std::vector<CTask*> tasks;

void NRtos::AddTask(CTask& task) {
    tasks.push_back(&task);
}

void NRtos::StartRtos() {
    for (CTask* task : tasks) {
        LOG_INF("Starting task %s", task->GetName());
        task->Initialize();
    }

    LOG_INF("RTOS Started!");
}

void NRtos::StopRtos() {
    for (CTask* task : tasks) {
        LOG_INF("Stopping task %s", task->GetName());
        task->~CTask();
    }

    LOG_INF("RTOS Stopped!");
}
