#include "n_boost.hpp"
#include <zephyr/kernel.h>
#include "common.hpp"
#include "f_core/utils/circular_buffer.hpp"
namespace NBoost {

CCircularBuffer<float, NUM_SAMPLES_OVER_BOOST_THRESHOLD_REQUIRED> buffer{0};
bool isDetected = false;

void FeedDetector(float accelVertical) {
    buffer.AddSample(accelVertical);
    bool detected = true;
    for (unsigned int i = 0; i < buffer.Size(); i++) {
        if (buffer[i] < BOOST_DETECT_THRESHOLD_MS2) {
            detected = false;
            break;
        }
    }
    isDetected = detected;
}
bool IsDetected() { return isDetected; }
} // namespace NBoost
