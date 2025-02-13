// All in one holder for all the detection we do
#include "f_core/util/debouncer.hpp"
#include "f_core/util/linear_fit.hpp"
#include "flight.hpp"

#include <n_autocoder_types.h>

class CDetectionHandler {
  public:
    static constexpr std::size_t window_size = 10; // @100hz, 1 second window.
    using VelocityFinder = CRollingSum<LinearFitSample<double>, window_size>;

    // Boost Detectors
    using AccBoostDetector = CDebouncer<ThresholdDirection::Over, double>;
    using BaromBoostDetector = CDebouncer<ThresholdDirection::Over, double>;

    using BaromNoseoverDetector = CDebouncer<ThresholdDirection::Under, double>;
    using BaromGroundDetector = CDebouncer<ThresholdDirection::Under, double>;
    CDetectionHandler(SensorModulePhaseController &controller)
        : controller(controller),
          primary_imu_boost_squared_detector(boost_time_thresshold, boost_threshold_m_s2 * boost_threshold_m_s2),
          secondary_imu_boost_squared_detector{boost_time_thresshold, boost_threshold_m_s2 * boost_threshold_m_s2},

          primary_barom_velocity_finder{LinearFitSample<double>{0, 0}},
          secondary_barom_velocity_finder{LinearFitSample<double>{0, 0}},

          primary_barom_noseover_detector{noseover_time_thresshold, noseover_velocity_thresshold},
          secondary_barom_noseover_detector{noseover_time_thresshold, noseover_velocity_thresshold},
          primary_barom_ground_detector{ground_time_thresshold, ground_velocity_thresshold},
          secondary_barom_ground_detector{ground_time_thresshold, ground_velocity_thresshold} {}

    SensorModulePhaseController &controller;
    AccBoostDetector primary_imu_boost_squared_detector;
    AccBoostDetector secondary_imu_boost_squared_detector;

    VelocityFinder primary_barom_velocity_finder;
    VelocityFinder secondary_barom_velocity_finder;

    BaromNoseoverDetector primary_barom_noseover_detector;
    BaromNoseoverDetector secondary_barom_noseover_detector;

    BaromGroundDetector primary_barom_ground_detector;
    BaromGroundDetector secondary_barom_ground_detector;

    static constexpr uint64_t BOOST_NOT_YET_HAPPENED = ~0;
    uint64_t boost_detected_time = BOOST_NOT_YET_HAPPENED;

    void HandleData(uint64_t uptime, const NTypes::SensorData &data);

    void HandleBoost(uint64_t uptime, const NTypes::SensorData &data);
    void HandleNoseover(uint32_t t_plus_ms, const NTypes::SensorData &data);
    void HandleGround(uint32_t t_plus_ms, const NTypes::SensorData &data);
};
