/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef C_TASK_H
#define C_TASK_H

#include <cstdint>
#include <vector>

#include <f_core/os/c_tenant.h>

class CTask {
public:
    CTask(const char* name, int priority, int stack_size, uint64_t time_slice);

    void AddTenant(CTenant &tenant);

    void Run();

private:
    std::vector<CTenant*> tenants;
};

#endif //C_TASK_H
