/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "f_core/os/n_rtos.h"

#include <string>
#include <zephyr/logging/log.h>
#include <zephyr/task_wdt/task_wdt.h>
LOG_MODULE_REGISTER(NRtos);

std::vector<CTask*> tasks;

static CTask* FindTaskByName(const std::string& taskName) {
    for (CTask* task : tasks) {
        if (taskName == task->GetName()) {
            return task;
        }
    }

    return nullptr;
}

void NRtos::AddTask(CTask& task) { tasks.push_back(&task); }

void NRtos::StartRtos() {
#ifdef CONFIG_F_CORE_TASK_WATCHDOG
    const device* hardwareWatchdog = nullptr;
#if defined(CONFIG_TASK_WDT_HW_FALLBACK) && defined(CONFIG_WATCHDOG) && DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(watchdog0))
    hardwareWatchdog = DEVICE_DT_GET(DT_ALIAS(watchdog0));
    if (!device_is_ready(hardwareWatchdog)) {
        LOG_WRN("Hardware watchdog not ready. Using software watchdog only");
        hardwareWatchdog = nullptr;
    }
#endif

    if (int ret = task_wdt_init(hardwareWatchdog); ret != 0) {
        LOG_ERR("Failed to initialize task watchdog: %d", ret);
    }
#endif

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

    LOG_RAW("RTOS Stopped!\n");
    LOG_RAW("\n");
}

void NRtos::ResumeTask(k_tid_t taskId) { k_thread_resume(taskId); }

void NRtos::ResumeTask(const std::string& taskName) {
    if (CTask* task = FindTaskByName(taskName)) {
        k_thread_resume(task->GetTaskId());
    } else {
        LOG_WRN("Cannot resume %s, because task was not found in map!", taskName.c_str());
    }
}

void NRtos::SuspendTask(k_tid_t taskId) { k_thread_suspend(taskId); }

void NRtos::SuspendTask(const std::string& taskName) {
    if (CTask* task = FindTaskByName(taskName)) {
        k_thread_suspend(task->GetTaskId());
    } else {
        LOG_WRN("Cannot suspend %s, because task was not found in map!", taskName.c_str());
    }
}

void NRtos::SuspendCurrentTask() {
    k_tid_t taskId = k_current_get();
    SuspendTask(taskId);
}
