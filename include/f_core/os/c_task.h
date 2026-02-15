/*
* Copyright (c) 2024 RIT Launch Initiative
*
* SPDX-License-Identifier: Apache-2.0
*/
#pragma once

#include <vector>

#include <f_core/os/c_runnable_tenant.h>
#include <zephyr/kernel.h>

/**
 * Thread to be ran in the RTOS
 */
class CTask {
public:
    /**
     * Constructor
     * @param name Name of the task
     * @param priority Zephyr priority level
     * @param stackSize Size of the stack to allocate
     * @param sleepTimeMs Time to sleep between a single cycle of the task
     */
    CTask(const char* name, int priority = CONFIG_NUM_PREEMPT_PRIORITIES, int stackSize = 512, int sleepTimeMs = 0);

    /**
     * Destructor
     */
    ~CTask();

    /**
     * Initialize the necessary components for the task and starts it
     */
    void Initialize();

    /**
     * Bind a tenant to the task
     * @param tenant Tenant to bind to a task
     */
    void AddTenant(CRunnableTenant& tenant);

    /**
     * @brief Run through the tenants and execute their Run method
     */
    void Run();

    /**
     * Get the Zephyr thread associated with the task
     * @return Zephyr thread
     */
    k_thread GetThread()
    {
        return this->thread;
    };

    k_tid_t GetTaskId() {
        return this->taskId;
    }

    /**
     * Get the name of the task
     * @return Name of the task
     */
    const char *GetName()
    {
        return this->name;
    };


private:
    const char* name;
    const int priority;
    const size_t stackSize;
    const int sleepTimeMs;
    k_tid_t taskId;
    k_thread thread;
    k_thread_stack_t* stack;

    std::vector<CRunnableTenant*> tenants;
};
