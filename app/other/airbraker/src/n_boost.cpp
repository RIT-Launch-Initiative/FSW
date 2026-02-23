#include "n_boost.hpp"
#include <zephyr/kernel.h>
#include "common.hpp"
#include "f_core/utils/circular_buffer.hpp"
namespace NBoost {

const float threshold_ms2 = 9.8 * 10;

CCircularBuffer<float, NUM_SAMPLES_OVER_BOOST_THRESHOLD_REQUIRED> buffer{0};
bool is_detected = false;

void FeedDetector(float accel_vertical) {
    buffer.AddSample(accel_vertical);
    bool detected = true;
    for (unsigned int i = 0; i < buffer.Size(); i++) {
        if (buffer[i] < threshold_ms2) {
            detected = false;
            break;
        }
    }
    is_detected = detected;
}
bool IsDetected() { return is_detected; }
} // namespace NBoost
