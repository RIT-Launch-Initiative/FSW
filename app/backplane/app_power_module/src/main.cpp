/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "c_power_module.h"

// F-Core Includes
#include <f_core/os/n_rtos.h>

int main() {
    static CPowerModule config{};

    NRtos::StartRtos();

    return 0;
}

