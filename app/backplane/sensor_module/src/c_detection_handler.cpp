#include "c_detection_handler.h"

#include <math.h>

double asl_from_pressure(double kpa) {
    double mbar = kpa * 10;
    return (1.0 - pow(mbar / 1013.25, 0.190284)) * 145366.45;
}

void CDetectionHandler::HandleData(uint64_t timestamp, const NTypes::SensorData& data) {
    double t_seconds = (double) timestamp / 1000.f;
    // printk("T: %.3f\n", (double) t_seconds);
    double primary_barom_asl = asl_from_pressure(data.PrimaryBarometer.Pressure);
    double secondary_barom_asl = asl_from_pressure(data.SecondaryBarometer.Pressure);

    primary_barom_velocity_finder.Feed(LinearFitSample(t_seconds, primary_barom_asl));
    secondary_barom_velocity_finder.Feed(LinearFitSample(t_seconds, secondary_barom_asl));
    // printf("%.2f, %.2f\n", t_seconds, primary_barom_asl);
    if (!controller.HasEventOccured(Events::Boost)) {
        HandleBoost(timestamp, data);
    }
    if (controller.HasEventOccured(Events::Boost) && boost_detected_time == BOOST_NOT_YET_HAPPENED) {
        boost_detected_time = timestamp;
    }
    // Don't worry about the rest until we've got boost
    if (!controller.HasEventOccured(Events::Boost)) {
        return;
    }

    uint32_t t_plus_ms = timestamp - boost_detected_time - boost_time_thresshold;
    double slope = 0;
    find_slope(primary_barom_velocity_finder, slope);

    // printf("%.2f, %.2f, %.2f, %.2f\n", (t_plus_ms / 1000.0), primary_barom_asl, secondary_barom_asl, slope);
    if (!controller.HasEventOccured(Events::Noseover)) {
        HandleNoseover(t_plus_ms, data);
    }
    if (controller.HasEventOccured(Events::Noseover) && !controller.HasEventOccured(Events::GroundHit)) {
        HandleGround(t_plus_ms, data);
    }
}

void CDetectionHandler::HandleGround(uint32_t t_plus_ms, const NTypes::SensorData& data) {
    double primary_barom_velocity = 0;
    double secondary_barom_velocity = 0;

    bool primary_good = find_slope(primary_barom_velocity_finder, primary_barom_velocity);
    if (primary_good) {
        primary_barom_ground_detector.feed(t_plus_ms, fabs(primary_barom_velocity));
    }

    bool secondary_good = find_slope(secondary_barom_velocity_finder, secondary_barom_velocity);
    if (secondary_good) {
        secondary_barom_ground_detector.feed(t_plus_ms, fabs(secondary_barom_velocity));
    }

    if (primary_barom_ground_detector.passed()) {
        controller.SubmitEvent(Sources::BaromMS5611, Events::GroundHit);
    }
    if (secondary_barom_ground_detector.passed()) {
        controller.SubmitEvent(Sources::BaromBMP, Events::GroundHit);
    }
}

void CDetectionHandler::HandleNoseover(uint32_t t_plus_ms, const NTypes::SensorData& data) {
    double primary_barom_velocity = 0;
    double secondary_barom_velocity = 0;

    bool primary_good = find_slope(primary_barom_velocity_finder, primary_barom_velocity);
    if (primary_good) {
        primary_barom_noseover_detector.feed(t_plus_ms, primary_barom_velocity);
    }

    bool secondary_good = find_slope(secondary_barom_velocity_finder, secondary_barom_velocity);
    if (secondary_good) {
        secondary_barom_noseover_detector.feed(t_plus_ms, secondary_barom_velocity);
    }

    if (primary_barom_noseover_detector.passed()) {
        controller.SubmitEvent(Sources::BaromMS5611, Events::Noseover);
    }
    if (secondary_barom_noseover_detector.passed()) {
        controller.SubmitEvent(Sources::BaromBMP, Events::Noseover);
    }
}

void CDetectionHandler::HandleBoost(uint64_t timestamp, const NTypes::SensorData& data) {
    double primary_mag_squared_m_s2 = data.Acceleration.X * data.Acceleration.X +
                                      data.Acceleration.Y * data.Acceleration.Y +
                                      data.Acceleration.Z * data.Acceleration.Z;
    double secondary_mag_squared_m_s2 = data.ImuAcceleration.X * data.ImuAcceleration.X +
                                        data.ImuAcceleration.Y * data.ImuAcceleration.Y +
                                        data.ImuAcceleration.Z * data.ImuAcceleration.Z;

    // printk("Mag: %.2f\n", primary_mag_squared_m_s2);
    primary_imu_boost_squared_detector.feed(timestamp, primary_mag_squared_m_s2);
    secondary_imu_boost_squared_detector.feed(timestamp, secondary_mag_squared_m_s2);

    if (primary_imu_boost_squared_detector.passed()) {
        controller.SubmitEvent(Sources::HighGImu, Events::Boost);
    }
    if (secondary_imu_boost_squared_detector.passed()) {
        controller.SubmitEvent(Sources::LowGImu, Events::Boost);
    }

    if (controller.HasEventOccured(Events::Boost) && boost_detected_time == BOOST_NOT_YET_HAPPENED) {
        boost_detected_time = timestamp;
    }
}
