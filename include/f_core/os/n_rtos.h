/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef N_RTOS_H
#define N_RTOS_H

#include "f_core/os/c_task.h"

namespace NRtos {
    void AddTask(const CTask &task);

    void StartRtos();

    void StopRtos();

    static std::vector<CTask*, 10> tasks;

};



#endif //N_RTOS_H
