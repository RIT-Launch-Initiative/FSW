/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef N_RTOS_H
#define N_RTOS_H

#include "f_core/os/c_task.h"

namespace NRtos {
/**
 * Add a task to be scheduled on the RTOS
 * @param task Task to be scheduled on the RTOS
 */
void AddTask(CTask& task);

/**
 * Initialize and start all tasks added to the RTOS
 */
void StartRtos();

/**
 * Cleanup tasks and abort all added tasks
 */
void StopRtos();

/**
 * Suspend a task from executing
 * @param taskId The ID of the task to suspend
 */
void SuspendTask(k_tid_t taskId);

/**
 * Suspend the currently executing task
 */
void SuspendCurrentTask();

/**
 * Resume a task's execution
 * @param taskId The ID of the task to resume
 */
void ResumeTask(k_tid_t taskId);
};


#endif //N_RTOS_H
