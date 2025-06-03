#include "boost.h"

#include "buzzer.h"
#include "f_core/utils/linear_fit.hpp"
#include "flight.h"

#include <cmath>
#include <zephyr/kernel.h>

// we sample at 1000hz, how convenient
CMovingAverage<float, imuBoostTimeThreshold> acc_buf{0};

bool overriding_boost = false;
// Dummy easu
bool feed_boost_acc(k_ticks_t ts_ticks, const NTypes::AccelerometerData &xyz) {
    float mag_sqrd = xyz.X * xyz.X + xyz.Y * xyz.Y + xyz.Z * xyz.Z;
    acc_buf.Feed(mag_sqrd);
    // imuBoostThresholdMPerS2 * imuBoostThresholdMPerS2
    if (acc_buf.Avg() > (9.8 * 9.8 * 2)) {
        printk("Boosted fr\n");
        buzzer_tell(BuzzCommand::AllGood);
        return true;
    }
    if (overriding_boost) {
        printk("Boosted not fr\n");
        buzzer_tell(BuzzCommand::AllGood);
        return true;
    }
    return false;
}
bool feed_boost_barom(int64_t ts_us, float temp, float press) { return false; }
