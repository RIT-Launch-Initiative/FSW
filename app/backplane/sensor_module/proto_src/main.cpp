#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(sensor_mod, CONFIG_LOG_DEFAULT_LEVEL);

extern void flashLogEnable();
extern void flashLogDisable();

static int cmdLogOn(const shell* sh, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    flashLogEnable();
    shell_print(sh, "Flash logging enabled");

    return 0;
}

static int cmdLogOff(const shell* sh, size_t argc, char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    flashLogDisable();
    shell_print(sh, "Flash logging disabled");

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_log,
                               SHELL_CMD_ARG(on, NULL, "Enable sensor data logging to flash", cmdLogOn, 1, 0),
                               SHELL_CMD_ARG(off, NULL, "Disable sensor data logging to flash", cmdLogOff, 1, 0),
                               SHELL_SUBCMD_SET_END
    );

SHELL_CMD_REGISTER(log, &sub_log, "Sensor data logging commands", NULL);

extern void flashListenerInit();
extern void ethListenerInit();

int main() {
    flashListenerInit();
    ethListenerInit();

    return 0;
}
