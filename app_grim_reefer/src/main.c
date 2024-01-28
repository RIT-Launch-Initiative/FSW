/*
 * Copyright (c) 2023 Richie Sommers
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_GRIM_REEFER_LOG_LEVEL_DBG);

static int init(void) { return 0; }

int main(void) {
  if (init()) {
    return -1;
  }

  // Won't run if initializing the network stack failed
  while (true) {
    // convert_and_send();
    printk("main looping\n");
    k_sleep(K_MSEC(1000));
  }
  return 0;
}
