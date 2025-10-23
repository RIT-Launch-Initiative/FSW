#include "control.h"
#include "flash_storage.h"
#include "adc_reading.h"

#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(shell_cmds, LOG_LEVEL_INF);

static int cmd_test_start(const struct shell *shell, size_t argc, char **argv){
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "Starting test...");
    control_start_test();
    return 0;
}

static int cmd_test_stop(const struct shell *shell, size_t argc, char **argv){
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "Stopping test...");
    control_stop_test();
    return 0;
}

static int cmd_test_dump(const struct shell *shell, size_t argc, char **argv){
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "Dumping stored test data...");
    control_dump_data(shell);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_test,
    SHELL_CMD(start, NULL, "Start test", cmd_test_start),
    SHELL_CMD(stop, NULL, "Stop test", cmd_test_stop),
    SHELL_CMD(dump, NULL, "Dump all flash data", cmd_test_dump),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(test, &sub_test, "Solids Test Board control commands", NULL);