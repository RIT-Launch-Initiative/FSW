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

#ifdef BOARD_GRIM_REEFER
LOG_MODULE_REGISTER(main, CONFIG_APP_GRIM_REEFER_LOG_LEVEL_DBG);
#else
#define CONFIG_APP_GRIM_REEFER_LOG_LEVEL_DBG 1
LOG_MODULE_REGISTER(main, CONFIG_APP_GRIM_REEFER_LOG_LEVEL_DBG);
#endif

// #define FLASH_NODE DT_ALIAS(led0)
#define FLASH_NODE DT_ALIAS(storage)
const struct device *const flash = DEVICE_DT_GET(FLASH_NODE);

int32_t increment_file_int32(char *fname, int32_t *count) {
  int32_t ret;
  int32_t close_ret;
  struct fs_file_t file;

  // prepare file for usage
  fs_file_t_init(&file);
  ret = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
  if (ret < 0) {
    LOG_PRINTK("Failed to open %s: %d\n", fname, ret);
    return ret;
  }

  // get the counter from the file
  ret = fs_read(&file, count, sizeof(*count));
  if (ret < 0) {
    LOG_PRINTK("Failed to read counter: %d\n", ret);
    goto exit;
  }

  (*count)++;

  // go back to the start of the file
  ret = fs_seek(&file, 0, FS_SEEK_SET);
  if (ret < 0) {
    LOG_PRINTK("Failed to seek to start: %d\n", ret);
    goto exit;
  }

  // write the counter to the file
  ret = fs_write(&file, count, sizeof(*count));
  if (ret < 0) {
    LOG_PRINTK("Failed to write new count: %d\n", ret);
  }

exit:
  close_ret = fs_close(&file);
  if (close_ret < 0) {
    LOG_PRINTK("Failed to close %s: %d\n", fname, close_ret);
    return close_ret;
  }
  return ret;
}

static int init(void) { return 0; }

int main(void) {
  if (init()) {
    return -1;
  }

  int32_t ret = 0;

  int32_t boot_counter = -1;

  // Boilerplate: set up GPIO
  // if (!gpio_is_ready_dt(&led)) {
  // printf("GPIO is not ready\n");
  // return 0;
  // }

  // if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
  // printf("Unable to configure LED output pin\n");
  // return 0;
  // }

#ifdef CONFIG_CLEAR_STORAGE_PARTITION
  if (!device_is_ready(flash)) {
    printf("Device %s is not ready\n", flash->name);
    return 0;
  }
  flash_erase(flash, 0, DT_PROP(FLASH_NODE, size));
#endif

  ret = increment_file_int32("/lfs/boot_count", &boot_counter);
  if (ret >= 0) {
    LOG_PRINTK("Successfully read and updated boot counter: %d boots\n",
               boot_counter);
    printk("inked\n");

  } else {
    LOG_PRINTK("Failed to read file\n");
    printk("failed\n");
  }

  // Won't run if initializing the network stack failed
  while (true) {
    // convert_and_send();
    printk("main looping\n");
    k_sleep(K_MSEC(5000));
  }
  return 0;
}
