/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef N_RTOS_H
#define N_RTOS_H

#include "f_core/os/c_task.h"

#include <string>

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
 * Resume a task's execution based on task ID
 * @param taskId The ID of the task to resume
 */
void ResumeTask(k_tid_t taskId);

/**
 * Resume a task's execution based on task name
 * @param taskName The name of the task to resume
 */
void ResumeTask(std::string taskName);

/**
 * Suspend a task from executing based on task ID
 * @param taskId The ID of the task to suspend
 */
void SuspendTask(k_tid_t taskId);

/**
 * Suspend a task from execution based on task name
 * @param taskName The name of the task to resume
 */
void SuspendTask(std::string taskName);

/**
 * Suspend the currently executing task
 */
void SuspendCurrentTask();
};


#endif //N_RTOS_H
