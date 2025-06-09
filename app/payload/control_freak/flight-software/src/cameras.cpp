#include "flight.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
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

    fc->WaitUntilEvent(Events::GroundHit);
    camera_off(&ground_cam);
    LOG_INF("Down camera off, antenna cam still on");

    fc->WaitUntilEvent(Events::InitialInflation);
    k_msleep(10 * 60 * 1000);
    camera_off(&antenna_cam);
    camera_off(&antenna_cam);
    LOG_INF("Antenna cam off, will be back soon");

    return 0;
}