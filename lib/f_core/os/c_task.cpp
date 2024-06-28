/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Self Include
#include <f_core/os/c_task.h>

// F-Core Includes
#include <functional>
#include <f_core/os/c_tenant.h>

static void taskEntryWrapper(void* taskObj, void*, void*) {
    auto* task = static_cast<CTask*>(taskObj);
    int sleepTimeMs = task->GetSleepTimeInMillis();

    while (true) {
        task->Run();
        k_msleep(sleepTimeMs);

#if defined(CONFIG_ARCH_POSIX)
        k_cpu_idle(); // Refer to Zephyr's POSIX arch limitations documentation
#endif
    }
}

CTask::CTask(const char* name, int priority, int stackSize, int sleepTimeMs) : name(name),
    priority(priority), stackSize(stackSize), sleepTimeMs(sleepTimeMs) {
    stack = k_thread_stack_alloc(stackSize, 0);
}

CTask::~CTask() {
    k_thread_abort(&thread);
    k_thread_stack_free(stack);
}

void CTask::Initialize() {
    stack = k_thread_stack_alloc(stackSize, 0);
    if (stack == nullptr) {
        __ASSERT(true, "Failed to allocate stack for %s ", name);
        return;
    }
    taskId = k_thread_create(&thread, stack, stackSize, taskEntryWrapper, this, nullptr, nullptr, priority, 0, K_NO_WAIT);
    k_thread_name_set(taskId, name);
}

void CTask::AddTenant(CTenant& tenant) {
    tenants.push_back(&tenant);
}

void CTask::Run() {
    for (CTenant* tenant : tenants) {
        tenant->Run();
    }
}
