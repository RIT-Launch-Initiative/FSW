#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>
#include <zephyr/shell/shell.h>

/**
 * @brief Starts flash storage and ADC reading
 */
void control_start_test();

/**
 * @brief Stops flash storage and ADC reading
 */
void control_stop_test();

/**
 * @brief Reads and prints n number of ADC samples
 * @param shell Pointer to shell instance
 * @param num Number of samples to read
 */
void control_print_n(const struct shell *shell, int num);

/**
 * @brief Dumps all ADC data from flash storage
 * @param shell Pointer to shell instance
 */
void control_dump_data(const struct shell *shell);

/**
 * @brief Dumps one ADC test from flash storage
 * @param shell Pointer to shell instance
 * @param test_index The test number to dump
 */
void control_dump_one(const struct shell *shell, uint32_t test_index);

/**
 * @brief Clear all flash blocks
 * @param shell Pointer to shell instance
 */
void control_erase_all(const struct shell *shell);

/**
 * @brief Set ematch gpio high
 * @param shell Pointer to shell instance
 */
void control_set_ematch(const struct shell *shell);

/**
 * @brief Set ematch gpio low
 * @param shell Pointer to shell instance
 */
void control_stop_ematch(const struct shell *shell);

#endif // CONTROL_H