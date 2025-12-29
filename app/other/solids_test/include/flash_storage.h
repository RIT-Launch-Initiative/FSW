#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <zephyr/shell/shell.h>
#include <stdint.h>

/**
 * @brief Begin flash storage event
 * @param[in] calib_name Name of calibration to store. Defaults to "Test [#]" if empty or default string passed in
 * @param[in] terminal_test Whether test was triggered by terminal cmd or meep
 * @return 0 if successful
 */
int start_flash_storage(char calib_name[], bool terminal_test);

/**
 * @brief End flash storage event
 */
void stop_flash_storage();

/**
 * @brief Dumps one ADC test from flash storage
 * @param[in] shell Pointer to shell instance
 * @param[in] test_index The test number to dump
 * @return 0 if successful
 */
int flash_dump_one(const struct shell *shell, uint32_t test_index);

/**
 * @brief Dumps all ADC data from flash storage
 * @param[in] shell Pointer to shell instance
 * @return 0 if successful
 */
int flash_dump_all(const struct shell *shell);

/**
 * @brief Clear all flash blocks
 * @param[in] shell Pointer to shell instance
 * @return 0 if successful
 */
int flash_erase_all(const struct shell *shell);

extern struct k_msgq storage_control_queue;

#endif // FLASH_STORAGE_H