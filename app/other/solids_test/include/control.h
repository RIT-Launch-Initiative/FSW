#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>
#include <zephyr/shell/shell.h>

/**
 * @brief Starts flash storage and ADC reading
 * @param[in] calib_name Name of calibration to store in flash storage
 * @param[in] terminal_test Whether test was triggered by terminal cmd or meep
 */
void control_start_test(char calib_name[], bool terminal_test);

/**
 * @brief Stops flash storage and ADC reading
 */
void control_stop_test();

/**
 * @brief Reads and prints n number of ADC samples
 * @param[in] shell Pointer to shell instance
 * @param[in] num Number of samples to read
 */
void control_print_n(const struct shell *shell, int num);

/**
 * @brief Dumps all ADC data from flash storage
 * @param[in] shell Pointer to shell instance
 */
void control_dump_data(const struct shell *shell);

/**
 * @brief Dumps one ADC test from flash storage
 * @param[in] shell Pointer to shell instance
 * @param[in] test_index The test number to dump
 */
void control_dump_one(const struct shell *shell, uint32_t test_index);

/**
 * @brief Clear all flash blocks
 * @param[in] shell Pointer to shell instance
 */
void control_erase_all(const struct shell *shell);

/**
 * @brief Set ematch gpio high
 * @param[in] shell Pointer to shell instance
 */
void control_set_ematch(const struct shell *shell);

/**
 * @brief Set ematch gpio low
 * @param[in] shell Pointer to shell instance
 */
void control_stop_ematch(const struct shell *shell);

/**
 * @brief Get status of test
 * @return Whether a test is running or not
 */
bool control_get_test_status();

#endif // CONTROL_H