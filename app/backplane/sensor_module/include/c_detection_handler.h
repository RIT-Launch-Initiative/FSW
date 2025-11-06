// All in one holder for all the detection we do
#include "f_core/utils/debouncer.hpp"
#include "f_core/utils/linear_fit.hpp"
#include "flight.hpp"

#include <n_autocoder_types.h>
#include <f_core/n_alerts.h>
#include <f_core/messaging/c_message_port.h>

class CDetectionHandler {
public:
    struct SensorWorkings {
        bool primaryAccOk;
        bool secondaryAccOk;
        bool primaryBarometerOk;
        bool secondaryBarometerOk;
    };

    static constexpr std::size_t BAROM_VELOCITY_FINDER_WINDOW_SIZE = 10; // @100hz, 0.1 second window.
    using VelocityFinder = CRollingSum<LinearFitSample<double>, BAROM_VELOCITY_FINDER_WINDOW_SIZE>;

    // Boost Detectors
    using AccBoostDetector = CDebouncer<ThresholdDirection::Over, double>;
    using BaromBoostDetector = CDebouncer<ThresholdDirection::Over, double>;

    using BaromNoseoverDetector = CDebouncer<ThresholdDirection::Under, double>;
    using BaromGroundDetector = CDebouncer<ThresholdDirection::Under, double>;
    CDetectionHandler(SensorModulePhaseController& controller,
                      CMessagePort<NAlerts::AlertPacket>& alertMessagePort);

    SensorModulePhaseController& controller;
    AccBoostDetector primaryImuBoostSquaredDetector;

    VelocityFinder primaryBaromVelocityFinder;
    VelocityFinder secondaryBaromVelocityFinder;

    BaromNoseoverDetector primaryBaromNoseoverDetector;
    BaromNoseoverDetector secondaryBaromNoseoverDetector;

    BaromGroundDetector primaryBaromGroundDetector;
    BaromGroundDetector secondaryBaromGroundDetector;

    static constexpr uint64_t BOOST_NOT_YET_HAPPENED = ~0;
    uint64_t boost_detected_time = BOOST_NOT_YET_HAPPENED;

    /**
     * Process sensor information 
     * @param uptime uptime in milliseconds of the system
     * @param data sensor data from Sensing Tenant
     * @param workings a description of which sensors were read correctly
     */
    void HandleData(const uint64_t uptime, const NTypes::SensorData& data, const SensorWorkings& workings);

    /**
     * Process sensor information before boost has been detected
     * @param t_plus_ms milliseconds since boost
     * @param data sensor data from Sensing Tenant
     * @param workings a description of which sensors were read correctly
     */
    void HandleBoost(const uint64_t uptime, const NTypes::SensorData& data, const SensorWorkings& workings);
    /**
     * Process sensor information before the noseover has been reached
     * @param t_plus_ms milliseconds since boost
     * @param data sensor data from Sensing Tenant
     * @param workings a description of which sensors were read correctly
     */
    void HandleNoseover(const uint32_t t_plus_ms, const NTypes::SensorData& data, const SensorWorkings& workings);
    /**
     * Process sensor information before the ground has been hit
     * @param t_plus_ms milliseconds since boost
     * @param data sensor data from Sensing Tenant
     * @param workings a description of which sensors were read correctly
     */
    void HandleGround(const uint32_t t_plus_ms, const NTypes::SensorData& data, const SensorWorkings& workings);

    bool FlightOccurring() {
        // int uptime = k_uptime_get();
        // static constexpr int START_TIME = 10000; // 10 seconds
        // static constexpr int END_TIME = 30000; // 30 seconds

        // return START_TIME <= uptime && uptime <= END_TIME;
        return controller.HasEventOccurred(Events::Boost) && !controller.HasEventOccurred(Events::GroundHit);
    }

    bool FlightFinished() {
        // int uptime = k_uptime_get();
        // return uptime >= 30000;

        return controller.HasEventOccurred(Events::GroundHit);
    }

    /**
     * Whether or not the phase detector needs more data. True before ground hit, false after
     * @return true if the detection system wants more data, false if we're all finished
     */
    bool ContinueCollecting();

private:
    static constexpr NAlerts::AlertPacket boostNotification = {
        'L', 'A', 'U', 'N', 'C', 'H', 'b'};
    static constexpr NAlerts::AlertPacket noseoverNotification = {
        'L', 'A', 'U', 'N', 'C', 'H', 'n'};
    static constexpr NAlerts::AlertPacket landedNotification = {
        'L', 'A', 'U', 'N', 'C', 'H', 'l'};

    CMessagePort<NAlerts::AlertPacket>& alertMessagePort;
};
