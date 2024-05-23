/*
 * Copyright (c) 2023 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <launch_core/net/tftp.h>
#include <launch_core/os/fs.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_POWER_MODULE_LOG_LEVEL);

int main(void) {
    struct tftpc client;
    k_msleep(5000);

    l_fs_boot_count_check();

    l_tftp_init(&client, "10.0.0.0");
    l_tftp_put(&client, "test.txt");


    return 0;
}