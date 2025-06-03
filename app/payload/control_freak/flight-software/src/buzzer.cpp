#include "buzzer.h"

#include "5v_ctrl.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(buzzer);

K_MSGQ_DEFINE(buzzer_msgq, sizeof(BuzzCommand), 4, 1);

void buzzer_entry_point(void *, void *, void *);
void buzzer_tell(BuzzCommand cond) { k_msgq_put(&buzzer_msgq, &cond, K_MSEC(10)); }

/*
 * Each bit corresponds to 1/8th of a second. a 1 means the buzzer is going
 * and a 0 means the buzzer is silent
 * After a code is beeped out, it is followed by 2 seconds of silence to differentiate messages
*/

#define BEEPCODE_LENGTH 32

const uint32_t beepcodes[] = {
    [Silent] = 0b00000000000000000000000000000000,
    // [AllGood] = 0b10000000000000000000000000000000,
    [AllGood] = 0b11111111111111111111111100000000,
    [SensorTroubles] = 0b11000000000000000000000000000000,
    [BatteryWarning] = 0b1110001110001110001110000000000,
    [BatteryBad] = 0b1110001110001110000000000000000,
    [DataLocked] = 0b10101010101010101010101010101010,
};

// whether or not to continue beeping after one go
const bool beepcodes_continue[] = {
    [Silent] = false,        // doesnt really matter
    [AllGood] = false,       // say once then shut up
    [SensorTroubles] = true, // condition to scrub
    [BatteryWarning] = true, // condition to scrub
    [BatteryBad] = true,     // condition to scrub
    [DataLocked] = true,     // condition to scrub
};

void buzzer_entry_point(void *, void *, void *) {
    uint32_t frame = 0;
    BuzzCommand current_cond = BuzzCommand::Silent;
    uint32_t current_code = beepcodes[current_cond];
    bool needs_continue = true;
    bool continue_mask = true;
    while (true) {
        // Read new buzzer request
        BuzzCommand incoming;
        if (k_msgq_get(&buzzer_msgq, &incoming, K_NO_WAIT) == 0) {
            current_cond = incoming;
            current_code = beepcodes[current_cond];
            needs_continue = beepcodes_continue[current_cond];
            continue_mask = true;
            frame = 0;
        }
        // Set buzzer according to beepcode
        int buzzer_state = (current_code >> frame) & 0b1;
        rail_item_set(FiveVoltItem::Buzzer, buzzer_state && continue_mask);

        // Progress time and wrap around when needed
        frame++;
        if (frame >= BEEPCODE_LENGTH) {
            frame = 0;
            if (!needs_continue) {
                continue_mask = false;
            }
        }
        k_msleep(125);
    }
    return;
}
