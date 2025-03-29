// All in one holder for all the detection we do
#include "f_core/utils/debouncer.hpp"
#include "f_core/utils/linear_fit.hpp"
#include "flight.hpp"

#include <n_autocoder_types.h>
#include <f_core/n_alerts.h>
#include <f_core/device/c_gpio.h>
#include <f_core/messaging/c_message_port.h>
#include <f_core/utils/c_soft_timer.h>
#include <zephyr/drivers/gpio.h>

static void disableLogging(k_timer *timer) {
    bool *allowLogging = static_cast<bool *>(k_timer_user_data_get(timer));
    *allowLogging = false;
}

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
    CDetectionHandler(SensorModulePhaseController &controller, CMessagePort<char[NAlerts::ALERT_PACKET_SIZE]>& alertMessagePort);

    SensorModulePhaseController &controller;
    AccBoostDetector primaryImuBoostSquaredDetector;
    AccBoostDetector secondaryImuBoostSquaredDetector;

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

    bool allowLogging = false;

private:
    CMessagePort<char[NAlerts::ALERT_PACKET_SIZE]>& alertMessagePort;
    CSoftTimer stopLoggingAfterGroundHitTimer{disableLogging};
    CGpio led0 = CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios));
    CGpio led1 = CGpio(GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios));

};
