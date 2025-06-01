#include "5v_ctrl.h"

#include <initializer_list>
#include <tuple>
#include <zephyr/drivers/gpio.h>

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
            printk("No %s pin :(\n", name);
        }
        int ret = gpio_pin_configure_dt(gpio, GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            printk("Failed to conf %s pin:(\n", name);
            return ret;
        }
    }
    printk("5V Rail all good\n");
    return 0;
}

static bool state[(int) FiveVoltItem::NumItems] = {false};

static int set_accordingly() { return gpio_pin_set_dt(&ldo_en, (state[0] || state[1] || state[2])); }

// enable a zone (if everything else is off, turn the rail on)
// NOTE: this does not turn on the buzzer/pump, just enables power to it
int rail_item_enable(FiveVoltItem item) { return rail_item_set(item, true); }

// disable a zone (if everything else is off, turn the rail off)
// NOTE: this does not turn off the buzzer/pump, just disables power to it
int rail_item_disable(FiveVoltItem item) { return rail_item_set(item, false); }

int rail_item_set(FiveVoltItem item, bool set) {
    state[(int) item] = set;
    return set_accordingly();
}
