#include "boost.h"

#include "buzzer.h"
#include "f_core/utils/linear_fit.hpp"

#include <cmath>
#include <zephyr/kernel.h>

CMovingAverage<float, 250> acc_buf{0};

bool overriding_boost = false;
// Dummy easu
bool feed_boost_acc(int64_t ts_us, float *xyz) {
    acc_buf.Feed(std::abs(xyz[1]));

    if (acc_buf.Avg() > 12) {
        printk("Boosted fr");
        buzzer_tell(BuzzCommand::AllGood);
        return true;
    }
    if (overriding_boost) {
        printk("Boosted not fr");
        buzzer_tell(BuzzCommand::AllGood);
        return true;
    }
    return false;
}
bool feed_boost_barom(int64_t ts_us, float temp, float press) { return false; }
