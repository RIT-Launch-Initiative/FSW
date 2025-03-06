// All in one holder for all the detection we do
#include "f_core/utils/debouncer.hpp"
#include "f_core/utils/linear_fit.hpp"
#include "flight.hpp"

#include <n_autocoder_types.h>

class CDetectionHandler {
  public:
    struct SensorWorkings {
        bool primary_acc_ok;
        bool secondary_acc_ok;
        bool primary_barometer_ok;
        bool secondary_barometer_ok;
    };
    static constexpr std::size_t window_size = 10; // @100hz, 0.1 second window.
    using VelocityFinder = CRollingSum<LinearFitSample<double>, window_size>;

    // Boost Detectors
    using AccBoostDetector = CDebouncer<ThresholdDirection::Over, double>;
    using BaromBoostDetector = CDebouncer<ThresholdDirection::Over, double>;

    using BaromNoseoverDetector = CDebouncer<ThresholdDirection::Under, double>;
    using BaromGroundDetector = CDebouncer<ThresholdDirection::Under, double>;
    CDetectionHandler(SensorModulePhaseController &controller);

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

    /**
     * Process sensor information 
     * @param uptime uptime in milliseconds of the system
     * @param data sensor data from Sensing Tenant
     * @param workings a description of which sensors were read correctly
     */
    void HandleData(const uint64_t uptime, const NTypes::SensorData &data, const SensorWorkings &workings);

    /**
     * Process sensor information before boost has been detected
     * @param t_plus_ms milliseconds since boost
     * @param data sensor data from Sensing Tenant
     * @param workings a description of which sensors were read correctly
     */
    void HandleBoost(const uint64_t uptime, const NTypes::SensorData &data, const SensorWorkings &workings);
    /**
     * Process sensor information before the noseover has been reached
     * @param t_plus_ms milliseconds since boost
     * @param data sensor data from Sensing Tenant
     * @param workings a description of which sensors were read correctly
     */
    void HandleNoseover(const uint32_t t_plus_ms, const NTypes::SensorData &data, const SensorWorkings &workings);
    /**
     * Process sensor information before the ground has been hit
     * @param t_plus_ms milliseconds since boost
     * @param data sensor data from Sensing Tenant
     * @param workings a description of which sensors were read correctly
     */
    void HandleGround(const uint32_t t_plus_ms, const NTypes::SensorData &data, const SensorWorkings &workings);

    /**
     * Whether or not the phase detector needs more data. True before ground hit, false after
     * @return true if the detection system wants more data, false if we're all finished
     */
    bool ContinueCollecting();
};
