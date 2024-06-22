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
#include <zephyr/kernel.h>

class CTask
{
public:
    CTask(const char* name, int priority, int stack_size);
    CTask::CTask(const char* name, int priority = 20, int stackSize = 512, k_timeout_t schedulingDelay = K_MSEC(50));

    ~CTask();

    void Initialize();

    void AddTenant(CTenant& tenant);

    void Run();

    k_thread GetThread()
    {
        return this->thread;
    };

private:
    const char* name;
    const int priority;
    const size_t stackSize;
    const k_timeout_t schedulingDelay;
    k_tid_t taskId;
    k_thread thread;
    k_thread_stack_t* stack;

    std::vector<CTenant*> tenants;
};

#endif //C_TASK_H
