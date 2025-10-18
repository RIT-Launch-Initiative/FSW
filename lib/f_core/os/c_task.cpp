/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Self Include
#include <f_core/os/c_task.h>

// F-Core Includes
#include "zephyr/drivers/watchdog.h"

#include <functional>
#include <f_core/os/c_tenant.h>
#include <zephyr/logging/log.h>

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

CTask::CTask(const char* name, int priority, int stackSize, int sleepTimeMs,
             wdt_timeout_cfg* wdtConfig) : name(name),
                                                     priority(priority), stackSize(stackSize),
                                                     sleepTimeMs(sleepTimeMs), wdtConfig(wdtConfig) {}

CTask::~CTask() {
    for (CRunnableTenant* tenant : tenants) {
        tenant->Cleanup();
    }

    k_thread_abort(&thread);
    if (stack != nullptr) {
        k_thread_stack_free(stack);
        stack = nullptr;
    }
}

void CTask::Initialize(const device* wdgDev) {
    if (wdtConfig != nullptr && wdgDev != nullptr) {
        watchdogDev = wdgDev;
        wdtTimeoutId = wdt_install_timeout(watchdogDev, wdtConfig);
    } else if (wdgDev != nullptr) {
        LOG_ERR("Watchdog device provided but no configuration specified");
        k_panic();
    } else {
        LOG_DBG("No watchdog configuration or device provided, skipping watchdog setup");
    }

    for (CRunnableTenant* tenant : tenants) {
        tenant->Startup();
    }

    for (CRunnableTenant* tenant : tenants) {
        tenant->PostStartup();
    }

    stack = k_thread_stack_alloc(stackSize, 0);
    if (stack == nullptr) {
        LOG_ERR("Failed to allocate stack for %s", name);
        k_panic();
    }

    taskId = k_thread_create(&thread, stack, stackSize, taskEntryWrapper, this, nullptr, nullptr, priority, 0,
                             K_NO_WAIT);

    k_thread_name_set(taskId, name);
}

void CTask::AddTenant(CRunnableTenant& tenant) {
    tenants.push_back(&tenant);
}

void CTask::Run() {
    for (CRunnableTenant* tenant : tenants) {
        tenant->Run();
    }

    if (wdtTimeoutId >= 0) {
        wdt_feed(watchdogDev, wdtTimeoutId);
    }

    k_msleep(sleepTimeMs);
}
