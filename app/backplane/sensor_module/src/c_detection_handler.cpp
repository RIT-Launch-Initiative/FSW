#include "c_detection_handler.h"

#include <math.h>

double asl_from_pressure(double P_sta_kpa) {
    // Constants from https://www.weather.gov/media/epz/wxcalc/pressureAltitude.pdf
    static constexpr double standard_atmosphere_exponent = 0.190284;
    static constexpr double standard_atmosphere_factor = 145366.45;

    static constexpr double kpa_to_mbar = 10.0;
    static constexpr double sea_level_pressure_mbar = 1013.25;
    double P_sta_mbar = P_sta_kpa * kpa_to_mbar;

    return (1 - pow(P_sta_mbar / sea_level_pressure_mbar, standard_atmosphere_exponent)) * standard_atmosphere_factor;
}
CDetectionHandler::CDetectionHandler(SensorModulePhaseController& controller)
    : controller(controller),
      primaryImuBoostSquaredDetector(boostTimeThreshold, boostThresholdMPerS2 * boostThresholdMPerS2),
      secondaryImuBoostSquaredDetector{boostTimeThreshold, boostThresholdMPerS2 * boostThresholdMPerS2},

      primaryBaromVelocityFinder{LinearFitSample<double>{0, 0}},
      secondaryBaromVelocityFinder{LinearFitSample<double>{0, 0}},

      primaryBaromNoseoverDetector{noseoverTimeThreshold, noseoverVelocityThresshold},
      secondaryBaromNoseoverDetector{noseoverTimeThreshold, noseoverVelocityThresshold},
      primaryBaromGroundDetector{groundTimeThreshold, groundVelocityThreshold},
      secondaryBaromGroundDetector{groundTimeThreshold, groundVelocityThreshold} {}

bool CDetectionHandler::ContinueCollecting() { return !controller.HasEventOccured(Events::GroundHit); }

void CDetectionHandler::HandleData(const uint64_t timestamp, const NTypes::SensorData& data,
                                   const SensorWorkings& sensor_states) {
    double t_seconds = static_cast<double>(timestamp) / 1000.0;

    double primary_barom_asl = asl_from_pressure(data.PrimaryBarometer.Pressure);
    double secondary_barom_asl = asl_from_pressure(data.SecondaryBarometer.Pressure);

    primaryBaromVelocityFinder.Feed(LinearFitSample(t_seconds, primary_barom_asl));
    secondaryBaromVelocityFinder.Feed(LinearFitSample(t_seconds, secondary_barom_asl));

    if (!controller.HasEventOccured(Events::Boost)) {
        HandleBoost(timestamp, data, sensor_states);
    }
    // Don't worry about the rest until we've got boost
    if (!controller.HasEventOccured(Events::Boost)) {
        return;
    }

    uint32_t t_plus_ms = timestamp - boost_detected_time - boostTimeThreshold;

    if (!controller.HasEventOccured(Events::Noseover)) {
        HandleNoseover(t_plus_ms, data, sensor_states);
    }
    if (controller.HasEventOccured(Events::Noseover) && !controller.HasEventOccured(Events::GroundHit)) {
        HandleGround(t_plus_ms, data, sensor_states);
    }
}

void CDetectionHandler::HandleGround(const uint32_t t_plus_ms, const NTypes::SensorData& data,
                                     const SensorWorkings& sensor_states) {
    double primary_barom_velocity = 0;
    double secondary_barom_velocity = 0;

    bool primary_good = FindSlope(primaryBaromVelocityFinder, primary_barom_velocity);
    if (primary_good) {
        primaryBaromGroundDetector.feed(t_plus_ms, fabs(primary_barom_velocity));
    }

    bool secondary_good = FindSlope(secondaryBaromVelocityFinder, secondary_barom_velocity);
    if (secondary_good) {
        secondaryBaromGroundDetector.feed(t_plus_ms, fabs(secondary_barom_velocity));
    }

    if (primaryBaromGroundDetector.passed() && sensor_states.primaryBarometerOk) {
        controller.SubmitEvent(Sources::BaromMS5611, Events::GroundHit);
    }
    if (secondaryBaromGroundDetector.passed() && sensor_states.secondaryBarometerOk) {
        controller.SubmitEvent(Sources::BaromBMP, Events::GroundHit);
    }
}

void CDetectionHandler::HandleNoseover(const uint32_t t_plus_ms, const NTypes::SensorData& data,
                                       const SensorWorkings& sensor_states) {
    double primary_barom_velocity = 0;
    double secondary_barom_velocity = 0;

    bool primary_vel_good = FindSlope(primaryBaromVelocityFinder, primary_barom_velocity);
    if (primary_vel_good) {
        primaryBaromNoseoverDetector.feed(t_plus_ms, primary_barom_velocity);
    }

    bool secondary_vel_good = FindSlope(secondaryBaromVelocityFinder, secondary_barom_velocity);
    if (secondary_vel_good) {
        secondaryBaromNoseoverDetector.feed(t_plus_ms, secondary_barom_velocity);
    }

    if (primaryBaromNoseoverDetector.passed() && controller.HasEventOccured(Events::NoseoverLockout) &&
        sensor_states.primaryBarometerOk) {
        controller.SubmitEvent(Sources::BaromMS5611, Events::Noseover);
    }
    if (secondaryBaromNoseoverDetector.passed() && controller.HasEventOccured(Events::NoseoverLockout) &&
        sensor_states.secondaryBarometerOk) {
        controller.SubmitEvent(Sources::BaromBMP, Events::Noseover);
    }
}

void CDetectionHandler::HandleBoost(const uint64_t timestamp, const NTypes::SensorData& data,
                                    const SensorWorkings& sensor_states) {
    double primary_mag_squared_m_s2 = data.ImuAcceleration.X * data.ImuAcceleration.X +
                                      data.ImuAcceleration.Y * data.ImuAcceleration.Y +
                                      data.ImuAcceleration.Z * data.ImuAcceleration.Z;
    double secondary_mag_squared_m_s2 = data.Acceleration.X * data.Acceleration.X +
                                        data.Acceleration.Y * data.Acceleration.Y +
                                        data.Acceleration.Z * data.Acceleration.Z;

    primaryImuBoostSquaredDetector.feed(timestamp, primary_mag_squared_m_s2);
    secondaryImuBoostSquaredDetector.feed(timestamp, secondary_mag_squared_m_s2);

    if (primaryImuBoostSquaredDetector.passed() && sensor_states.primaryAccOk) {
        controller.SubmitEvent(Sources::HighGImu, Events::Boost);
    }
    if (secondaryImuBoostSquaredDetector.passed() && sensor_states.secondaryAccOk) {
        controller.SubmitEvent(Sources::LowGImu, Events::Boost);
    }

    if (controller.HasEventOccured(Events::Boost) && boost_detected_time == BOOST_NOT_YET_HAPPENED) {
        boost_detected_time = timestamp;
    }
}
