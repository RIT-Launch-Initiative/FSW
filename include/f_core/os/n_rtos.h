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
    void AddTask(CTask &task);

    /**
     * Initialize and start all tasks added to the RTOS
     */
    void StartRtos();

    /**
     * Cleanup tasks and abort all added tasks
     */
    void StopRtos();
};


#endif //N_RTOS_H
