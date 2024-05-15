#include "buzzer.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(buzzer);

K_MSGQ_DEFINE(buzzer_msgq, sizeof(enum buzzer_cond), 10, 1);

void buzzer_tell(enum buzzer_cond cond) { k_msgq_put(&buzzer_msgq, &cond, K_MSEC(10)); }

#define BUZZER_THREAD_STACK_SIZE 500
#define BUZZER_THREAD_PRIORITY   10

// 2 second beepcode, 2 second pause
const uint16_t beepcodes[] = {
    [buzzer_cond_noflash] = 0b1010101010101010,
    [buzzer_cond_low_battery] = 0b1110001110001110,
    [buzzer_cond_missing_sensors] = 0b1100000011000000,
    [buzzer_cond_landed] = 0b1111111100000000,
    [buzzer_cond_ok] = 0b0000000000000000,
};

extern void buzzer_entry_point(void *buzzer_gpio, void *, void *) {
    const struct gpio_dt_spec *buzzer = buzzer_gpio;
    uint32_t frame = 0;
    enum buzzer_cond current_cond = buzzer_cond_ok;
    uint16_t current_code = beepcodes[current_cond];
    while (1) {
        // Read new buzzer request
        enum buzzer_cond incoming;
        if (k_msgq_get(&buzzer_msgq, &incoming, K_NO_WAIT) == 0) {
            // New message - is it more important than the current?
            if (incoming < current_cond) {
                current_cond = incoming;
                current_code = beepcodes[current_cond];
                frame = 0;
            }
        }
        // Set buzzer according to beepcode
        if (frame < 16) {
            int buzzer_state = (current_code >> frame) & 0b1;
            gpio_pin_set_dt(buzzer, buzzer_state);
        } else {
            gpio_pin_set_dt(buzzer, 0);
        }

        // Progress time
        frame++;
        if (frame == 32) {
            frame = 0;
        }
        k_msleep(125);
    }
    return;
}

K_THREAD_STACK_DEFINE(buzzer_stack_area, BUZZER_THREAD_STACK_SIZE);
struct k_thread buzzer_thread_data;

void begin_buzzer_thread(const struct gpio_dt_spec *buzzer_pin) {
    k_tid_t my_tid =
        k_thread_create(&buzzer_thread_data, buzzer_stack_area, K_THREAD_STACK_SIZEOF(buzzer_stack_area),
                        buzzer_entry_point, (void *) buzzer_pin, NULL, NULL, BUZZER_THREAD_PRIORITY, 0, K_NO_WAIT);
}
