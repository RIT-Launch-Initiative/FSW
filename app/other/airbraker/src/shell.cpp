#include "common.hpp"
#include "n_model.hpp"
#include "servo.hpp"
#include <cmath>
#include <zephyr/shell/shell.h>

#define BAILOUT_IF_NOT_CANCELLED(shell)                                                                                \
    if (!IsFlightCancelled()) {                                                                                        \
        shell_error(shell, "MUST CANCEL FLIGHT BEFORE EXECUTING THIS FUNCTION");                                       \
        return -1;                                                                                                     \
    }

static int cmd_nogo(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    if (IsFlightCancelled()){
        shell_info(shell, "Flight already cancelled");
        return 0;
    }
    shell_error(shell, "Cancelling flight. MUST REBOOT TO START DETECTION AGAIN");
    shell_error(shell, "To Reboot: cycle power or execute 'kernel reboot'");
    CancelFlight();
    shell_prompt_change(shell, "!nogo!$ ");
    return 0;
}

static int cmd_describe_params(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Airbrakes - " CONFIG_BOARD " - %s %s", __DATE__, __TIME__);
    shell_print(shell, "Model Version: %s", NModel::GetMatlabLUTName());
    shell_print(shell, "Flight: =================================");
    shell_print(shell, "Lockout:            %u ms", LOCKOUT_MS);
    shell_print(shell, "Flight Time:        %u s", FLIGHT_TIME_MS);
    shell_print(shell, "Flight Length:      %u pkts", NUM_FLIGHT_PACKETS);

    shell_print(shell, "Pre-Flight: ==============================");
    shell_print(shell, "Preboost Length:    %u pkts", NUM_STORED_PREBOOST_PACKETS);
    shell_print(shell, "Bias Samples:       %u pkts", NUM_SAMPLES_FOR_GYRO_BIAS);

    shell_print(shell, "Boost Detect: ============================");
    shell_print(shell, "Threshold:          %.2f m/s² (%.2f g)", (double) BOOST_DETECT_THRESHOLD_MS2,
                (double) (BOOST_DETECT_THRESHOLD_MS2) / 9.8);
    shell_print(shell, "Count:              %u samples", NUM_SAMPLES_OVER_BOOST_THRESHOLD_REQUIRED);

    return 0;
}

bool parse_long(const char *str, long *out) {
    int *endptr = nullptr;
    long val = shell_strtol(str, 10, endptr);
    if (endptr != nullptr) {
        return false;
    } else {
        *out = val;
        return true;
    }
}

static int cmd_servo_step(const struct shell *shell, size_t argc, char **argv) {
    BAILOUT_IF_NOT_CANCELLED(shell);

    if (argc != 3 && argc != 4) {
        shell_error(shell, "Usage: test servo_step [startEffort] effort hold_time_ms");
        shell_error(shell, "If unspecified, startEffort = 0. Effort in units of 1000: 500 -> 0.5. Clamped to [0, 1]");
        return -1;
    }
    long startEffort = 0;
    long effort = 1000;  // filled by parse
    long holdTimeMS = 0; // filled by parse

    const char *effortStr = (argc == 3) ? argv[1] : argv[2];
    const char *holdTimeStr = (argc == 3) ? argv[2] : argv[3];

    if (argc == 4) {
        const char *startEffortStr = argv[1];
        bool parsed = parse_long(startEffortStr, &startEffort);
        if (!parsed || startEffort > 1000) {
            shell_error(shell, "invalid start effort value. Needs range [0, 1000]");
            return -1;
        }
    }
    bool parsed = parse_long(effortStr, &effort);
    if (!parsed || effort > 1000) {
        shell_error(shell, "invalid effort value. Needs range [0, 1000]");
        return -1;
    }
    parsed = parse_long(holdTimeStr, &holdTimeMS);
    if (!parsed) {
        shell_error(shell, "invalid hold time value. Needs integer milliseconds");
        return -1;
    }

    shell_info(shell, "1. Enabling Servo");
    EnableServo();

    SetServoEffort((float) startEffort / 1000.0F);
    shell_info(shell, "2. Move to initial position");
    k_msleep(2000);
    shell_info(shell, "3. Setting effort");
    SetServoEffort((float) effort / 1000.0F);
    k_msleep(holdTimeMS);

    shell_info(shell, "4. Disabling Servo");
    DisableServo();

    return 0;
}

static int cmd_servo_wave(const struct shell *shell, size_t argc, char **argv) {
    BAILOUT_IF_NOT_CANCELLED(shell);

    if (argc != 3) {
        shell_error(shell, "Usage: test servo_wave periodMS num_cycles. Make the servo do a sine wave from 0 to 1 and "
                           "back num_cycles times with a period of periodMS");
        return -1;
    }

    long periodMS = 0; // time for in and out = periodMS
    long cycleNum = 0; // number of cycles to repeat for
    if (!parse_long(argv[1], &periodMS)){
        shell_error(shell, "failed to parse periodMS. need integer number of milliseconds");
        return -1;
    }
    if (!parse_long(argv[2], &cycleNum)){
        shell_error(shell, "failed to parse num_cycles. need integer number of cycles");
        return -1;
    }

    EnableServo();

    for (long i = 0; i < cycleNum; i++){
        for (long j = 0; j < periodMS; j++){
            float t = (float)j / (float)periodMS;
            float effort = (1-cosf(2 * (float)M_PI *t))/2;
            SetServoEffort(effort);
            k_msleep(1);
        }
    }


    DisableServo();

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(subcmds, SHELL_CMD(nogo, NULL, "Cancel main mission", cmd_nogo),
                               SHELL_CMD(info, NULL, "Program Information", cmd_describe_params), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(airbrake, &subcmds, "Airbrake Control Commands", NULL);

SHELL_STATIC_SUBCMD_SET_CREATE(test_cmds,
                               SHELL_CMD(servo_wave, NULL, "Make a sine wave with the servo", cmd_servo_wave),
                               SHELL_CMD(servo_step, NULL, "Do a step function on the servo", cmd_servo_step),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(test, &test_cmds, "Airbrake Test Commands", NULL);
