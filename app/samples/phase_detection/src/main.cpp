// F-Core Includes
#include "f_core/util/debouncer.hpp"
#include "f_core/util/linear_fit.hpp"

#include <f_core/device/sensor/c_accelerometer.h>
#include <f_core/device/sensor/c_barometer.h>
#include <f_core/device/sensor/c_gyroscope.h>
#include <f_core/device/sensor/c_magnetometer.h>
#include <math.h>
#include <zephyr/kernel.h>

// Zephyr Includes
#include "flight.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_PHASE_DETECT_LOG_LEVEL);

#include "f_core/util/debouncer.hpp"

#include <array>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

K_TIMER_DEFINE(imu_timer, NULL, NULL);
K_TIMER_DEFINE(barom_timer, NULL, NULL);

Controller controller{sourceNames, eventNames, timer_events, deciders};

void imu_thread_f(void *, void *, void *) {
    // > 5G-ish for a quarter second
    constexpr uint32_t boost_time_ms = 250;
    constexpr double boost_threshold_mps2 = 50;

    // < 3.5G-ish for a quarter second
    // weird since this example is doing what grim did and doing it based on magnitude not a specific axis
    // Deceleration is still acceleration which is why we cant just say < 1G
    constexpr uint32_t coast_time_ms = 250;
    constexpr double coast_threshold_mps2 = 35;
    Debuouncer<ThresholdDirection::Over, double> boost_detector{boost_time_ms, boost_threshold_mps2};
    Debuouncer<ThresholdDirection::Under, double> coast_detector{coast_time_ms, coast_threshold_mps2};

    CAccelerometer acc(*DEVICE_DT_GET_ONE(openrocket_imu));
    if (!acc.IsReady()) {
        LOG_WRN("Accelerometer not ready");
    }
    controller.SubmitEvent(Sources::IMU1, Events::PadReady);
    controller.WaitUntilEvent(Events::PadReady);

    while (true) {
        k_timer_status_sync(&imu_timer);
        uint32_t timestamp = k_uptime_get();
        bool good = acc.UpdateSensorValue();
        if (!good) {
            LOG_ERR("Failure reading imu");
        }
        double x = acc.GetSensorValueDouble(SENSOR_CHAN_ACCEL_X);
        double y = acc.GetSensorValueDouble(SENSOR_CHAN_ACCEL_Y);
        double z = acc.GetSensorValueDouble(SENSOR_CHAN_ACCEL_Z);
        double mag = sqrt(x * x + y * y + z * z);

        // Boost Detecting
        if (!controller.HasEventOccured(Events::Boost)) {
            boost_detector.feed(timestamp, mag);
            coast_detector.feed(timestamp, mag);
            if (boost_detector.passed()) {
                controller.SubmitEvent(Sources::IMU1, Events::Boost);
            }
        } else if (!controller.HasEventOccured(Events::Coast)) {
            coast_detector.feed(timestamp, mag);
            if (coast_detector.passed()) {
                controller.SubmitEvent(Sources::IMU1, Events::Coast);
            }
        }
    }
}
K_THREAD_DEFINE(imu_thread, 1024, imu_thread_f, NULL, NULL, NULL, 0, 0, 0);

using SampleType = LinearFitSample<double>;
static constexpr std::size_t window_size = 10;
using SummerType = RollingSum<SampleType, window_size>;

struct Line {
    double m;
    double b;
    constexpr bool operator==(const Line &rhs) const { return m == rhs.m && b == rhs.b; }
};
// Value returned when we don't have enough information to fit a line
constexpr Line bad_line{0, 0};

Line find_line(const SummerType &summer) {
    std::size_t N = summer.size();
    SampleType E = summer.sum();
    double denom = (N * E.xx - E.x * E.x);
    if (denom == 0) {
        // printf("Would have divided by 0\n");
        return bad_line;
    }
    double m = (N * E.xy - E.x * E.y) / denom;
    double b = (E.y - m * E.x) / N;
    return Line{m, b};
}

double l_altitude_conversion(double pressure_kpa, double temperature_c) {
    double pressure = pressure_kpa * 10;
    double altitude = (1 - pow(pressure / 1013.25, 0.190284)) * 145366.45 * 0.3048;
    return altitude;
}

using BoostDebouncerT = Debuouncer<ThresholdDirection::Over, double>;
using NoseoverDebouncerT = Debuouncer<ThresholdDirection::Under, double>;
using MainHeightDebouncerT = Debuouncer<ThresholdDirection::Under, double>;
using NoVelocityDebouncerT = Debuouncer<ThresholdDirection::Under, double>;

void barom_thread_f(void *, void *, void *) {
    CBarometer barometer(*DEVICE_DT_GET_ONE(openrocket_barometer));
    if (!barometer.IsReady()) {
        LOG_WRN("Accelerometer not ready");
    }
    controller.SubmitEvent(Sources::Barom1, Events::PadReady);
    controller.WaitUntilEvent(Events::PadReady);

    SummerType velocity_summer{SampleType{0, 0}};

    constexpr uint32_t boost_time = 2000;   // ms
    constexpr uint32_t boost_threshold = 5; // ft/s
    BoostDebouncerT boost_debouncer{boost_time, boost_threshold};

    // Under 0ft/s for 0.5 seconds
    constexpr uint32_t noseover_velocity_time = 500;
    constexpr double noseover_velocity_threshold = 0.0;
    NoseoverDebouncerT noseover_debouncer{noseover_velocity_time, noseover_velocity_threshold};

    // Under 500ft for 0.5 seconds
    constexpr uint32_t mainheight_time = 500;
    constexpr double mainheight_threshold = 200.0;
    MainHeightDebouncerT mainheight_debouncer{mainheight_time, mainheight_threshold};

    // under 5 ft/s for 10 seconds
    constexpr uint32_t no_vel_time = 10 * 1000;
    constexpr double no_vel_threshold = 5.0;
    NoVelocityDebouncerT no_vel_debouncer{no_vel_time, no_vel_threshold};

    while (true) {
        k_timer_status_sync(&barom_timer);
        bool good = barometer.UpdateSensorValue();
        if (!good) {
            LOG_ERR("Failure reading barometer");
        }

        // Measure
        double temp = barometer.GetSensorValueDouble(SENSOR_CHAN_AMBIENT_TEMP);
        double press = barometer.GetSensorValueDouble(SENSOR_CHAN_PRESS);

        uint32_t time_ms = k_uptime_get();
        double time_s = (double) (time_ms) / 1000.0;

        // Calculate
        double feet = l_altitude_conversion(press, temp);
        double feet_agl = feet - 10;

        velocity_summer.feed({time_s, feet});
        Line line = find_line(velocity_summer);

        // LOG_DBG("Speed: %f", feet);

        // Not enough samples, don't check
        if (line == bad_line) {
            continue;
        }
        double velocity_ft_s = line.m;

        // Check
        if (!controller.HasEventOccured(Events::Boost)) {
            boost_debouncer.feed(time_ms, velocity_ft_s);
            if (boost_debouncer.passed()) {
                controller.SubmitEvent(Sources::Barom1, Events::Boost);
            }
        } else if (!controller.HasEventOccured(Events::Noseover)) {
            noseover_debouncer.feed(time_ms, velocity_ft_s);
            if (noseover_debouncer.passed()) {
                controller.SubmitEvent(Sources::Barom1, Events::Noseover);
            }
        } else if (!controller.HasEventOccured(Events::MainChute)) {
            mainheight_debouncer.feed(time_ms, feet_agl);
            if (mainheight_debouncer.passed()) {
                controller.SubmitEvent(Sources::Barom1, Events::MainChute);
            }
        } else if (!controller.HasEventOccured(Events::GroundHit)) {
            no_vel_debouncer.feed(time_ms, fabs(velocity_ft_s));
            if (no_vel_debouncer.passed()) {
                controller.SubmitEvent(Sources::Barom1, Events::GroundHit);
            }
        }
    }
}
K_THREAD_DEFINE(barom_thread, 1024, barom_thread_f, NULL, NULL, NULL, 0, 0, 0);

int main() {
    k_timer_stop(&imu_timer);
    k_timer_stop(&barom_timer);

    LOG_DBG("Waiting until everyone ready");

    controller.WaitUntilEvent(Events::PadReady);
    LOG_DBG("System ready:\n\tstart boost detecting");

    // Start sensing
    k_timer_start(&imu_timer, K_MSEC(1), K_MSEC(1));
    k_timer_start(&barom_timer, K_MSEC(10), K_MSEC(10));

    controller.WaitUntilEvent(Events::Boost);
    LOG_DBG("Boost detected:\n\ttell your friends (engineering cams)");

    controller.WaitUntilEvent(Events::Coast);
    LOG_DBG("Coast detected:\n\tturn down IMU data rate");

    // IMU can chill out
    // k_timer_start(&imu_timer, K_MSEC(10), K_MSEC(10));

    // Mock Noseover
    // controller.SubmitEvent(Sources::Barom1, Events::Noseover);

    controller.WaitUntilEvent(Events::Noseover);
    LOG_DBG("Noseover detected:\n\tdeploy charges");

    controller.WaitUntilEvent(Events::MainChute);
    LOG_DBG("Main Chute Deploy:\n\tdeploy more charges");

    controller.WaitUntilEvent(Events::GroundHit);
    LOG_DBG("Hit The ground: Stop\n\trecording data");

    // Stop recording
    k_timer_stop(&imu_timer);
    k_timer_stop(&barom_timer);

    controller.WaitUntilEvent(Events::CamerasOff);
    LOG_DBG("Flight over:\n\tTurn off cameras");
    return 0;
}
