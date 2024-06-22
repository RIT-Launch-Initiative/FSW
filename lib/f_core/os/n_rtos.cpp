/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <f_core/os/n_rtos.h>

void NRtos::AddTask(const CTask &task) {
    tasks.push_back(task);
}

void NRtos::StartRtos() {
}

void NRtos::StopRtos() {
    // Stop RTOS
}

