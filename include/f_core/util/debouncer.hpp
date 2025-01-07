#ifndef F_CORE_UTIL_DEBOUNCER_
#define F_CORE_UTIL_DEBOUNCER_
#include <stdint.h>
enum class ThresholdDirection {
    Over,
    Under,
};

template <ThresholdDirection direction, typename Scalar = float, typename Timestamp = uint32_t>
class CDebouncer {
  public:
    constexpr CDebouncer(Timestamp duration, Scalar target_value) : duration(duration), target_value(target_value) {}
    constexpr void feed(Timestamp t, Scalar new_value) {
        if (passesOne(new_value)) {
            lastTimePassed = t;
            if (firstTimePassed == NOT_PASSED) {
                firstTimePassed = t;
            }
        } else {
            firstTimePassed = NOT_PASSED;
            lastTimePassed = NOT_PASSED;
        }
    }
    constexpr bool passed() {
        if (firstTimePassed == NOT_PASSED || lastTimePassed == NOT_PASSED) {
            return false;
        }
        return (lastTimePassed - firstTimePassed) > duration;
    }

  private:
    constexpr bool passesOne(Scalar new_value) {
        if constexpr (direction == ThresholdDirection::Over) {
            return new_value > target_value;
        } else {
            return new_value < target_value;
        }
    }
    Timestamp duration;
    Scalar target_value;

    static constexpr Timestamp NOT_PASSED = (Timestamp) ~0;

    Timestamp firstTimePassed = NOT_PASSED;
    Timestamp lastTimePassed = NOT_PASSED;
};

#endif