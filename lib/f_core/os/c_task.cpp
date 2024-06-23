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

    while (true) {
        task->Run();
    }
}

CTask::CTask(const char* name, int priority, int stackSize, k_timeout_t schedulingDelay) : name(name),
    priority(priority), stackSize(stackSize), schedulingDelay(schedulingDelay) {
    stack = k_thread_stack_alloc(stackSize, 0);
}

CTask::~CTask() {
    k_thread_abort(&thread);
    k_thread_stack_free(stack);
}

void CTask::Initialize() {
    stack = k_thread_stack_alloc(stackSize, 0);
    if (stack == NULL) {
        __ASSERT(true, "Failed to allocate stack for %s ", name);
        return;
    }
    taskId = k_thread_create(&thread, stack, stackSize, taskEntryWrapper, this, NULL, NULL, priority, 0, schedulingDelay);
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
