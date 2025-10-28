#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <zephyr/shell/shell.h>
#include <stdint.h>

int start_flash_storage();
void stop_flash_storage();
int flash_dump_all(const struct shell *shell);
int flash_dump_one(const struct shell *shell, uint32_t test_index);
int flash_erase_all(const struct shell *shell);

extern struct k_msgq storage_control_queue;

#endif // FLASH_STORAGE_H