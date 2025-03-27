/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/device/c_rtc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);


int main() {
    device *rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

    return 0;
}
