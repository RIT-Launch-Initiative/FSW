#ifndef REEFER_INLCUDE_BUZZER_H
#define REEFER_INLCUDE_BUZZER_H

#include <zephyr/drivers/gpio.h>

// Closer to zero get precedence
// If you have missing sensors but low battery, the low battery beepcode will be announced
enum buzzer_cond {
    buzzer_cond_noflash = 0,
    buzzer_cond_landed = 1,
    buzzer_cond_launched = 2,
    buzzer_cond_low_battery = 3,
    buzzer_cond_missing_sensors = 4,
    buzzer_cond_ok = 5,

};

/**
 * @brief Signal to the buzzer that an event of interest has happened
 * @param cond the condition the user should be aware of. It will get 
 * beeped out if it is a higher precedence message than that which is 
 * already being beeped out 
 */
void buzzer_tell(enum buzzer_cond cond);
/**
 * @brief Begin the buzzer thread after gpio's have been setup
 * @param buzzer_pin the gpio pin that goes to the buzzer transistor
*/
void begin_buzzer_thread(const struct gpio_dt_spec *buzzer_pin);

#endif