#include "adc_reading.h"
#include "control.h"
#include "config.h"
#include "flash_storage.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(shell_cmds, LOG_LEVEL_INF);

static int cmd_test_start(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    char calib_name[CALIB_NAME_MAX_LEN] = "default"; // If no name arg, set to default. Name will be set to "Test [#]" in flash_storage
    if (argc >= 2) {
        calib_name[0] = '\0'; // clear "default"
        for (int i = 1; i < argc; i++) {
            snprintf(calib_name + strlen(calib_name), 
                    sizeof(calib_name) - strlen(calib_name), 
                    i == 1 ? "%s" : " %s", argv[i]);
        }
    }

    shell_print(shell, "Starting test...");
    control_start_test(calib_name, true);
    return 0;
}

static int cmd_test_print_one(const struct shell *shell, size_t argc, char **argv) {
    int num = 1;
    if (argc == 2) {
        num = atoi(argv[1]);
    }
    shell_print(shell, "Getting %d sample(s)...", num);
    control_print_n(shell, num);
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
    shell_print(shell, "Erasing all test data...");

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

static int cmd_test_ematch(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "Setting ematch...");
    control_set_ematch(shell);
    return 0;
}

static int cmd_test_estop(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "Stopping ematch...");
    control_stop_ematch(shell);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_test, SHELL_CMD(start, NULL, "Start test. Arg [calibration name]", cmd_test_start),
                               SHELL_CMD(stop, NULL, "Stop test preemptively", cmd_test_stop),
                               SHELL_CMD(dump, NULL, "Dump flash data. Optional arg [test #]", cmd_test_dump),
                               SHELL_CMD(erase, NULL, "Erase all flash data, prepare for new tests", cmd_test_erase),
                               SHELL_CMD(read, NULL, "Read one (or more) samples", cmd_test_print_one),
                               SHELL_CMD(ematch, NULL, "Set ematch high", cmd_test_ematch),
                               SHELL_CMD(estop, NULL, "Set ematch low", cmd_test_estop),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(test, &sub_test, "Solids Test Board control commands", NULL);