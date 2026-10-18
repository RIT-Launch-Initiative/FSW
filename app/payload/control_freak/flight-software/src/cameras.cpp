#include "flight.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(cameras);

static constexpr uint32_t millis_off = (15 * 60 * 1000);
static constexpr uint32_t millis_on = (40 * 1000);
static constexpr float no_cam_voltage_thresshold = 7.9;

static float last_battery_voltage = 8.0;

int update_battery_voltage(const float &bat_voltage);
bool should_turnon_cam() { return last_battery_voltage > no_cam_voltage_thresshold; }

int camera_on(const struct gpio_dt_spec *cam) { return gpio_pin_set_dt(cam, 1); }
int camera_off(const struct gpio_dt_spec *cam) { return gpio_pin_set_dt(cam, 0); }

static const struct gpio_dt_spec antenna_cam = GPIO_DT_SPEC_GET(DT_ALIAS(antennacam), gpios);
static const struct gpio_dt_spec ground_cam = GPIO_DT_SPEC_GET(DT_ALIAS(antennacam), gpios);

void both_cams_on() {
    camera_on(&antenna_cam);
    camera_on(&ground_cam);
}
void both_cams_off() {
    camera_off(&antenna_cam);
    camera_off(&ground_cam);
}

int camera_thread_entry(void *v_fc, void *, void *) {
    FreakFlightController *fc = static_cast<FreakFlightController *>(v_fc);

    int ret = gpio_pin_configure_dt(&antenna_cam, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_WRN("Couldnt configure antennacam");
    }
    ret = gpio_pin_configure_dt(&ground_cam, GPIO_OUTPUT);
    if (ret != 0) {
        LOG_WRN("Couldnt configure antennacam");
    }

    camera_off(&antenna_cam);
    camera_off(&ground_cam);

    fc->WaitUntilEvent(Events::Boost);

    camera_on(&antenna_cam);
    camera_on(&ground_cam);
    LOG_INF("Both cameras on");

    fc->WaitUntilEvent(Events::InitialInflation);
    k_msleep(10 * 60 * 1000);
    camera_off(&antenna_cam);
    camera_off(&antenna_cam);
    LOG_INF("All cam off");

    return 0;
}

int cmd_upcam(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        shell_error(shell, "Need 0 or 1 to turn on or off");
        return -1;
    }
    bool turnon = false;
    if (argv[1][0] == '1') {
        turnon = true;
    } else if (argv[1][0] == '0') {
        turnon = false;
    } else {
        shell_error(shell, "Couldnt parse second arg");
        return -1;
    }
    if (turnon) {
        return camera_on(&antenna_cam);
    } else {
        return camera_off(&antenna_cam);
    }
}

int cmd_downcam(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        shell_error(shell, "Need 0 or 1 to turn on or off");
        return -1;
    }
    bool turnon = false;
    if (argv[1][0] == '1') {
        turnon = true;
    } else if (argv[1][0] == '0') {
        turnon = false;
    } else {
        shell_error(shell, "Couldnt parse second arg");
        return -1;
    }
    if (turnon) {
        return camera_on(&ground_cam);
    } else {
        return camera_off(&ground_cam);
    }
}

SHELL_STATIC_SUBCMD_SET_CREATE(cam_subcmds, SHELL_CMD(up, NULL, "Up Camera 0/1", cmd_upcam),
                               SHELL_CMD(down, NULL, "Down Camera 0/1", cmd_downcam), SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(cam, &cam_subcmds, "Camera Commands", NULL);
