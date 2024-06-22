/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Self Include
#include <f_core/os/c_task.h>

// F-Core Includes
#include <f_core/os/c_tenant.h>

CTask::CTask(const char* name, int priority, int stack_size, uint64_t time_slice) {
}

void CTask::AddTenant(const CTenant &tenant) {
    tenants.push_back(tenant);
}

void CTask::Run() {
    for (CTenant *tenant : tenants) {
        tenant->Run();
    }
}
