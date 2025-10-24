#include "adc_reading.h"
#include "control.h"
#include "flash_storage.h"

#include <stdint.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(shell_cmds, LOG_LEVEL_INF);

static int cmd_test_start(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "Starting test...");
    control_start_test();
    return 0;
}

static int cmd_test_stop(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "Stopping test...");
    control_stop_test();
    return 0;
}

static int cmd_test_erase(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "Erasing all test data......");

    control_erase_all(shell);
    return 0;
}
static int cmd_test_dump(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    if (argc == 1) {
        shell_print(shell, "Dumping all test data...");
        control_dump_data(shell);
    } else if (argc == 2) {
        uint32_t test_num = atoi(argv[1]);
        shell_print(shell, "Dumping test %u data...", test_num);
        control_dump_one(shell, test_num);
    } else {
        shell_print(shell, "Enter <test dump> or <test dump [number]>");
        return -1;
    }

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_test, SHELL_CMD(start, NULL, "Start test", cmd_test_start),
                               SHELL_CMD(stop, NULL, "Stop test preemptively", cmd_test_stop),
                               SHELL_CMD(dump, NULL, "Dump flash data. Optional arg [test #]", cmd_test_dump),
                               SHELL_CMD(erase, NULL, "Erase all flash data, prepare for new tests", cmd_test_erase),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(test, &sub_test, "Solids Test Board control commands", NULL);