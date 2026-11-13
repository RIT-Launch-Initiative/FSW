/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Self Include
#include <f_core/os/c_task.h>

// F-Core Includes
#include <f_core/os/c_tenant.h>
#include <functional>
#include <zephyr/logging/log.h>
#include <zephyr/task_wdt/task_wdt.h>

LOG_MODULE_REGISTER(CTask);

static void taskEntryWrapper(void* taskObj, void*, void*) {
    auto* task = static_cast<CTask*>(taskObj);

    while (true) {
        task->Run();
#if defined(CONFIG_ARCH_POSIX)
        // Refer to Zephyr's POSIX arch limitations documentation
        // https://docs.zephyrproject.org/latest/boards/native/native_sim/doc/index.html
        k_cpu_idle();
#endif
    }
}

CTask::CTask(const char* name, int priority, int stackSize, int sleepTimeMs, uint32_t watchdogTimeoutMs)
    : name(name), priority(priority), stackSize(stackSize), sleepTimeMs(sleepTimeMs),
      watchdogTimeoutMs(watchdogTimeoutMs) {}

CTask::~CTask() {
    for (CRunnableTenant* tenant : tenants) {
        tenant->Cleanup();
    }

#ifdef CONFIG_F_CORE_TASK_WATCHDOG
    if (watchdogChannel >= 0) {
        task_wdt_delete(watchdogChannel);
        watchdogChannel = -1;
    }
#endif

    k_thread_abort(&thread);
    if (stack != nullptr) {
        k_thread_stack_free(stack);
        stack = nullptr;
    }
}

void CTask::Initialize() {
    for (CRunnableTenant* tenant : tenants) {
        tenant->Startup();
    }

    for (CRunnableTenant* tenant : tenants) {
        tenant->PostStartup();
    }

    if (watchdogTimeoutMs > 0) {
#ifdef CONFIG_F_CORE_TASK_WATCHDOG
        watchdogChannel = task_wdt_add(watchdogTimeoutMs, nullptr, nullptr);
        if (watchdogChannel < 0) {
            LOG_ERR("Failed to add watchdog channel for %s: %d", name, watchdogChannel);
        }
#else
        LOG_WRN("Watchdog timeout set for %s, but CONFIG_F_CORE_TASK_WATCHDOG is disabled", name);
#endif
    }

    stack = k_thread_stack_alloc(stackSize, 0);
    if (stack == nullptr) {
        LOG_ERR("Failed to allocate stack for %s", name);
        k_panic();
    }

    taskId =
        k_thread_create(&thread, stack, stackSize, taskEntryWrapper, this, nullptr, nullptr, priority, 0, K_NO_WAIT);

    k_thread_name_set(taskId, name);
}

void CTask::AddTenant(CRunnableTenant& tenant) { tenants.push_back(&tenant); }

void CTask::Run() {
    for (CRunnableTenant* tenant : tenants) {
        tenant->Run();
    }

#ifdef CONFIG_F_CORE_TASK_WATCHDOG
    if (watchdogChannel >= 0) {
        task_wdt_feed(watchdogChannel);
    }
#endif

    k_msleep(sleepTimeMs);
}
