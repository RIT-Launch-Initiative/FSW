#include <zephyr/drivers/gpio.h>

// Closer to zero get precedence
enum buzzer_cond {
    buzzer_cond_noflash = 0,
    buzzer_cond_low_battery = 1,
    buzzer_cond_missing_sensors = 2,
    buzzer_cond_landed = 3,
    buzzer_cond_ok = 4,

};

void buzzer_tell(enum buzzer_cond cond);
void begin_buzzer_thread(const struct gpio_dt_spec *buzzer_pin);