#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <zephyr/shell/shell.h>
#include <stdint.h>

/**
 * @brief Begin flash storage event
 * @return 0 if message queue put successful
 */
int start_flash_storage();

/**
 * @brief End flash storage event
 */
void stop_flash_storage();

/**
 * @brief Dumps one ADC test from flash storage
 * @param shell Pointer to shell instance
 * @param test_index The test number to dump
 */
int flash_dump_one(const struct shell *shell, uint32_t test_index);

/**
 * @brief Dumps all ADC data from flash storage
 * @param shell Pointer to shell instance
 */
int flash_dump_all(const struct shell *shell);

/**
 * @brief Clear all flash blocks
 * @param shell Pointer to shell instance
 */
int flash_erase_all(const struct shell *shell);

extern struct k_msgq storage_control_queue;

#endif // FLASH_STORAGE_H