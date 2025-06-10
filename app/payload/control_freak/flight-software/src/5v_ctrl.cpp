#include "5v_ctrl.h"

#include <initializer_list>
#include <tuple>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rail5v);

static const struct gpio_dt_spec ldo_en = GPIO_DT_SPEC_GET(DT_ALIAS(ldo5v), gpios);

static const struct gpio_dt_spec servo_en = GPIO_DT_SPEC_GET(DT_NODELABEL(servo_enable), gpios);

static const struct gpio_dt_spec pump_enable = GPIO_DT_SPEC_GET(DT_NODELABEL(pump_enable), gpios);

const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(DT_ALIAS(buzz), gpios);

// Prepares the GPIO
int five_volt_rail_init() {
    using Pair = std::tuple<const struct gpio_dt_spec *, const char *>;
    constexpr Pair pairs[4] = {
        Pair{&ldo_en, "ldo5v"},
        Pair{&servo_en, "servo"},
        Pair{&pump_enable, "pump"},
        Pair{&buzzer, "buzzer"},
    };
    for (auto [gpio, name] : pairs) {
        if (!gpio_is_ready_dt(gpio)) {
            LOG_WRN("No %s pin :(", name);
        }
        int ret = gpio_pin_configure_dt(gpio, GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            LOG_WRN("Failed to conf %s pin :(", name);
            return ret;
        }
    }
    LOG_INF("5V Rail all good");
    return 0;
}

static bool state[(int) FiveVoltItem::NumItems] = {false};

static int set_ldo_accordingly() { return gpio_pin_set_dt(&ldo_en, (state[0] || state[1] || state[2])); }
static int set_item(FiveVoltItem item, bool set) {
    if (item >= FiveVoltItem::NumItems) {
        LOG_WRN("Invalid 5v rail item");
        return -ENODEV;
    }
    state[(int) item] = set;
    if (item == FiveVoltItem::Buzzer) {
        return gpio_pin_set_dt(&buzzer, set);
    } else if (item == FiveVoltItem::Servos) {
        return gpio_pin_set_dt(&servo_en, set);
    } else {
        return gpio_pin_set_dt(&pump_enable, set);
    }
    return 0;
}
// CONFIG_PM who?
int rail_item_enable(FiveVoltItem item) { return rail_item_set(item, true); }

int rail_item_disable(FiveVoltItem item) { return rail_item_set(item, false); }

int rail_item_set(FiveVoltItem item, bool set) {
    set_item(item, set);
    return set_ldo_accordingly();
}
