#include "boost.h"

#include "f_core/utils/linear_fit.hpp"

#include <cmath>

CMovingAverage<float, 250> acc_buf{0};

// Dummy easu
bool feed_boost_acc(int64_t ts_us, float *xyz) {
    acc_buf.Feed(std::abs(xyz[1]));
    if (acc_buf.Avg() > 8) {
        return true;
    }
    return false;
}
bool feed_boost_barom(int64_t ts_us, float temp, float press) { return false; }
