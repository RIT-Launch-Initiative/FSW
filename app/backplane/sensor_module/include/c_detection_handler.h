// All in one holder for all the detection we do
#include "f_core/util/debouncer.hpp"
#include "f_core/util/linear_fit.hpp"
#include "flight.hpp"

class CDetectionHandler {
    using AccBoostDetector = CDebouncer<ThresholdDirection::Under, float>;
    using BaromBoostDetector = CDebouncer<ThresholdDirection::Over, float>;

    using BaromNoseoverDetector = CDebouncer<ThresholdDirection::Under, float>;
    using BaromGroundDetector = CDebouncer<ThresholdDirection::Under, float>;

    AccBoostDetector primary_imu_boost{boost_threshold_m_s2, boost_time_thresshold};
    AccBoostDetector secondary_imu_boost{boost_threshold_m_s2, boost_time_thresshold};

    BaromNoseoverDetector primary_barom_noseover{noseover_velocity_thresshold, noseover_time_thresshold};
    BaromNoseoverDetector secondary_barom_noseover{noseover_velocity_thresshold, noseover_time_thresshold};

    BaromGroundDetector primary_ground_noseover{ground_velocity_thresshold, ground_time_thresshold};
    BaromGroundDetector secondary_ground_noseover{ground_velocity_thresshold, ground_time_thresshold};

    void HandleData(uint32_t timestamp, const SensorData* data);
};
