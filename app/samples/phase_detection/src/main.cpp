// F-Core Includes
#include "f_core/os/flight_log.hpp"
#include "f_core/utils/debouncer.hpp"
#include "f_core/utils/linear_fit.hpp"
#include "flight.h"

#include <array>
#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_gyroscope.h>
#include <f_core/device/sensor/c_magnetometer.h>
#include <math.h>
// Zephyr Includes
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_PHASE_DETECT_LOG_LEVEL);

K_TIMER_DEFINE(imu_timer, NULL, NULL);
K_TIMER_DEFINE(barom_timer, NULL, NULL);

void imu_thread_f(void *vp_controller, void *, void *) {
    Controller &controller = *(Controller *) vp_controller;

    // > 5G-ish for a quarter second
    constexpr uint32_t boost_time_ms = 250;
    constexpr double boost_threshold_mps2 = 50;

    // < 3.5G-ish for a quarter second
    // weird since this example is doing what grim did and doing it based on magnitude not a specific axis
    // Deceleration is still acceleration which is why we cant just say < 1G
    constexpr uint32_t coast_time_ms = 250;
    constexpr double coast_threshold_mps2 = 35;
    CDebouncer<ThresholdDirection::Over, double> boost_detector{boost_time_ms, boost_threshold_mps2};
    CDebouncer<ThresholdDirection::Under, double> coast_detector{coast_time_ms, coast_threshold_mps2};

    CAccelerometer acc(*DEVICE_DT_GET_ONE(openrocket_imu));
    if (!acc.IsReady()) {
        LOG_WRN("Accelerometer not ready");
    }
    controller.SubmitEvent(Sources::IMU1, Events::PadReady);
    controller.WaitUntilEvent(Events::PadReady);

    while (!controller.HasEventOccured(Events::GroundHit)) {
        k_timer_status_sync(&imu_timer);
        uint32_t timestamp = k_uptime_get();

        if (!acc.UpdateSensorValue()) {
            LOG_ERR("Failure reading imu");
        }
        double x = acc.GetSensorValueDouble(SENSOR_CHAN_ACCEL_X);
        double y = acc.GetSensorValueDouble(SENSOR_CHAN_ACCEL_Y);
        double z = acc.GetSensorValueDouble(SENSOR_CHAN_ACCEL_Z);
        double mag = sqrt((x * x) + (y * y) + (z * z));

        // Boost Detecting
        if (!controller.HasEventOccured(Events::Boost)) {
            boost_detector.Feed(timestamp, mag);
            coast_detector.Feed(timestamp, mag);
            if (boost_detector.Passed()) {
                controller.SubmitEvent(Sources::IMU1, Events::Boost);
            }
        } else if (!controller.HasEventOccured(Events::Coast)) {
            coast_detector.Feed(timestamp, mag);
            if (coast_detector.Passed()) {
                controller.SubmitEvent(Sources::IMU1, Events::Coast);
            }
        }
    }
}

using SampleType = LinearFitSample<double>;
static constexpr std::size_t window_size = 10;
using SummerType = CRollingSum<SampleType, window_size>;

struct Line {
    double m;
    double b;
    constexpr bool operator==(const Line &rhs) const { return m == rhs.m && b == rhs.b; }
};

// Value returned when we don't have enough information to fit a line
constexpr Line bad_line{0, 0};

Line find_line(const SummerType &summer) {
    std::size_t N = summer.Size();
    SampleType E = summer.Sum();
    double denom = (N * E.xx - E.x * E.x);
    if (denom == 0) {
        // Would have divided by 0
        return bad_line;
    }
    double m = (N * E.xy - E.x * E.y) / denom;
    double b = (E.y - m * E.x) / N;
    return Line{m, b};
}

double rrc3_altitude_conversion_to_feet(double P_sta_kpa) {
    // Constants from https://www.weather.gov/media/epz/wxcalc/pressureAltitude.pdf
    static constexpr double standard_atmosphere_exponent = 0.190284;
    static constexpr double standard_atmosphere_factor = 145366.45;

    static constexpr double kpa_to_mbar = 10.0;
    static constexpr double sea_level_pressure_mbar = 1013.25;
    double P_sta_mbar = P_sta_kpa * kpa_to_mbar;

    return (1 - pow(P_sta_mbar / sea_level_pressure_mbar, standard_atmosphere_exponent)) * standard_atmosphere_factor;
}

using BoostDebouncerT = CDebouncer<ThresholdDirection::Over, double>;
using NoseoverDebouncerT = CDebouncer<ThresholdDirection::Under, double>;
using MainHeightDebouncerT = CDebouncer<ThresholdDirection::Under, double>;
using NoVelocityDebouncerT = CDebouncer<ThresholdDirection::Under, double>;

void barom_thread_f(void *vp_controller, void *, void *) {
    Controller &controller = *(Controller *) vp_controller;

    CBarometer barometer(*DEVICE_DT_GET_ONE(openrocket_barometer));
    if (!barometer.IsReady()) {
        LOG_WRN("Accelerometer not ready");
    }
    controller.SubmitEvent(Sources::Barom1, Events::PadReady);
    controller.WaitUntilEvent(Events::PadReady);

    // Measure
    barometer.UpdateSensorValue();
    double init_press_kpa = barometer.GetSensorValueDouble(SENSOR_CHAN_PRESS);
    double init_feet_asl = rrc3_altitude_conversion_to_feet(init_press_kpa);
    double max_feet_agl = 0;

    SummerType velocity_summer{SampleType{0, 0}};
    CMovingAverage<double, 100> ground_level_avger{init_feet_asl};

    // >5g for 2 seconds
    constexpr uint32_t boost_time_ms = 2000; // ms
    constexpr uint32_t boost_threshold = 5;  // ft/s
    BoostDebouncerT boost_debouncer{boost_time_ms, boost_threshold};

    // Under 0ft/s for 0.5 seconds
    constexpr uint32_t noseover_velocity_time_ms = 500;
    constexpr double noseover_velocity_threshold = 0.0;
    NoseoverDebouncerT noseover_debouncer{noseover_velocity_time_ms, noseover_velocity_threshold};

    // Under 500ft for 0.5 seconds
    constexpr uint32_t mainheight_time_ms = 500;
    constexpr double mainheight_threshold = 500.0;
    MainHeightDebouncerT mainheight_debouncer{mainheight_time_ms, mainheight_threshold};

    // under 10 ft/s for 10 seconds
    constexpr uint32_t no_vel_time_ms = 10 * 1000;
    constexpr double no_vel_threshold = 10.0;
    NoVelocityDebouncerT no_vel_debouncer{no_vel_time_ms, no_vel_threshold};

    while (!controller.HasEventOccured(Events::GroundHit)) {
        k_timer_status_sync(&barom_timer);
        bool good = barometer.UpdateSensorValue();
        if (!good) {
            LOG_ERR("Failure reading barometer");
        }

        // Measure
        double press_kpa = barometer.GetSensorValueDouble(SENSOR_CHAN_PRESS);

        uint32_t time_ms = k_uptime_get();
        double time_s = (double) (time_ms) / 1000.0;

        // Calculate
        double feet = rrc3_altitude_conversion_to_feet(press_kpa);

        velocity_summer.Feed({time_s, feet});
        Line line = find_line(velocity_summer);

        // Not enough samples, don't check
        if (line == bad_line) {
            continue;
        }
        double velocity_ft_s = line.m;
        double feet_agl = feet - ground_level_avger.Avg();
        if (feet_agl > max_feet_agl) {
            max_feet_agl = feet_agl;
        }
        // Check
        if (!controller.HasEventOccured(Events::Boost)) {
            ground_level_avger.Feed(feet);
            boost_debouncer.Feed(time_ms, velocity_ft_s);
            if (boost_debouncer.Passed()) {
                controller.SubmitEvent(Sources::Barom1, Events::Boost);
            }
        }
        if (!controller.HasEventOccured(Events::Noseover)) {
            noseover_debouncer.Feed(time_ms, velocity_ft_s);
            if (controller.HasEventOccured(Events::Boost) && noseover_debouncer.Passed()) {
                controller.SubmitEvent(Sources::Barom1, Events::Noseover);
                if (controller.GetFlightLog() != nullptr) {
                    char print_buf[256] = {0};
                    int len = snprintf(print_buf, 256,
                                       "Noseover occured at barometeric altitude of %.2f ft agl (%.2f ft asl)",
                                       feet_agl, feet);
                    controller.GetFlightLog()->Write(print_buf, len);
                }
            }
        }
        if (!controller.HasEventOccured(Events::MainChute)) {
            mainheight_debouncer.Feed(time_ms, feet_agl);
            if (controller.HasEventOccured(Events::Noseover) && mainheight_debouncer.Passed()) {
                controller.SubmitEvent(Sources::Barom1, Events::MainChute);
            }
        }
        if (!controller.HasEventOccured(Events::GroundHit)) {
            no_vel_debouncer.Feed(time_ms, fabs(velocity_ft_s));
            if (controller.HasEventOccured(Events::Boost) && no_vel_debouncer.Passed()) {
                controller.SubmitEvent(Sources::Barom1, Events::GroundHit);
            }
        }
    }
    if (controller.GetFlightLog() != nullptr) {
        char print_buf[256] = {0};
        int len = snprintf(print_buf, 256, "Maximum barometric altitude of %.2f ft agl", max_feet_agl);

        controller.GetFlightLog()->Write(print_buf, len);
    }
}

K_THREAD_STACK_DEFINE(imu_thread_stack_area, 1024);
struct k_thread imu_thread_data;

K_THREAD_STACK_DEFINE(barom_thread_stack_area, 1024);
struct k_thread barom_thread_data;

int main() {
    CFlightLog fl{"/lfs/flight_log.txt"};
    Controller controller{sourceNames, eventNames, timer_events, deciders, &fl};

    k_thread_create(&barom_thread_data, barom_thread_stack_area, K_THREAD_STACK_SIZEOF(barom_thread_stack_area),
                    barom_thread_f, &controller, NULL, NULL, 0, 0, K_NO_WAIT);

    k_thread_create(&imu_thread_data, imu_thread_stack_area, K_THREAD_STACK_SIZEOF(imu_thread_stack_area), imu_thread_f,
                    &controller, NULL, NULL, 0, 0, K_NO_WAIT);
    k_timer_stop(&imu_timer);
    k_timer_stop(&barom_timer);

    LOG_DBG("Waiting until everyone ready");

    controller.WaitUntilEvent(Events::PadReady);
    LOG_DBG("System ready:\tstart boost detecting");

    // Start sensing
    k_timer_start(&imu_timer, K_MSEC(1), K_MSEC(1));
    k_timer_start(&barom_timer, K_MSEC(10), K_MSEC(10));

    LOG_DBG("Waiting for boost");
    controller.WaitUntilEvent(Events::Boost);
    LOG_DBG("Boost detected:\ttell your friends (engineering cams)");

    LOG_DBG("Waiting for coast");
    controller.WaitUntilEvent(Events::Coast);
    LOG_DBG("Coast detected:\tturn down IMU data rate");

    // IMU can chill out after all the cool stuff happens
    k_timer_start(&imu_timer, K_MSEC(10), K_MSEC(10));

    LOG_DBG("Waiting for noseover");
    controller.WaitUntilEvent(Events::Noseover);
    LOG_DBG("Noseover detected:\tdeploy charges");

    LOG_DBG("Waiting for main");
    controller.WaitUntilEvent(Events::MainChute);
    LOG_DBG("Main Chute Deploy:\tdeploy more charges");

    LOG_DBG("Waiting for ground");
    controller.WaitUntilEvent(Events::GroundHit);
    LOG_DBG("Hit The ground:\tStop recording data");

    // Stop recording
    k_timer_stop(&imu_timer);
    k_timer_stop(&barom_timer);

    controller.WaitUntilEvent(Events::CamerasOff);
    LOG_DBG("Flight over:\tTurn off cameras");

    fl.Sync();
    LOG_INF("Closed flight log");
    return 0;
}
