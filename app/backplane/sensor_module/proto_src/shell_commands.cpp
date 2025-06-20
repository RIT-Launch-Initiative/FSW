#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include "messages.h"

LOG_MODULE_REGISTER(shell_cmds, CONFIG_LOG_DEFAULT_LEVEL);

/* External functions from flash_listener.cpp */
extern void flashLogEnable(void);
extern void flashLogDisable(void);

/* Shell command to enable flash logging */
static int cmd_log_on(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    flashLogEnable();
    shell_print(sh, "Flash logging enabled");

    return 0;
}

/* Shell command to disable flash logging */
static int cmd_log_off(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    flashLogDisable();
    shell_print(sh, "Flash logging disabled");

    return 0;
}

/* Define the shell commands */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_log,
    SHELL_CMD_ARG(on, NULL, "Enable sensor data logging to flash", cmd_log_on, 1, 0),
    SHELL_CMD_ARG(off, NULL, "Disable sensor data logging to flash", cmd_log_off, 1, 0),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(log, &sub_log, "Sensor data logging commands", NULL);

/* Initialize the shell commands */
void shellCommandsInit(void)
{
    LOG_INF("Shell commands initialized");
}
