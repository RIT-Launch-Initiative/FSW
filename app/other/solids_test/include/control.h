#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>
#include <zephyr/shell/shell.h>

void control_init();
void control_start_test();
void control_stop_test();
void control_dump_data(const struct shell *shell);
void control_dump_one(const struct shell *shell, uint32_t test_index);
bool control_is_running();

#endif // CONTROL_H