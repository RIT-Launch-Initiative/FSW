#include "n_boost.h"
#include "n_model.h"
#include "n_preboost.hpp"
#include "n_sensing.hpp"
#include "n_storage.h"
#include "servo.h"

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_AIRBRAKE_LOG_LEVEL);

SYS_INIT(servo_init, APPLICATION, 1);
SYS_INIT(storage_init, APPLICATION, 2);

K_TIMER_DEFINE(measurement_timer, NULL, NULL);

uint32_t packet_timestamp() {
    int64_t ms = k_uptime_get();
    return (uint32_t) ms;
}

int main() {
    NSensing::init_sensors();
    k_timer_start(&measurement_timer, K_MSEC(10), K_MSEC(10));

    Packet packet{
        .timestamp = 0,
        .tempRaw = 0,
        .pressureRaw = 0,
        .accelRaw = 0,
        .kalmanState = {0},
        .gyro = {0},
        .orientationQuat = {1, 0, 0, 0},
        .effort = 0,
    };

    NSensing::MeasureSensors(packet.tempRaw, packet.pressureRaw, packet.accelRaw, packet.gyro);

    NBoost::FeedDetector(packet.accelRaw);

    float offset = NModel::AltitudeMetersFromPressureKPa(packet.pressureRaw);

    DisableServo();

    // boost detect time
    while (!NBoost::IsDetected()) {
        packet.timestamp = packet_timestamp();
        NSensing::MeasureSensors(packet.tempRaw, packet.pressureRaw, packet.accelRaw, packet.gyro);

        NBoost::FeedDetector(packet.accelRaw);

        float altMeters = NModel::AltitudeMetersFromPressureKPa(packet.pressureRaw) - offset;
        NModel::FeedKalman(packet.timestamp, altMeters, packet.accelRaw);

        packet.kalmanState = NModel::LastKalmanState();
        packet.effort = 0; // no fun until after burnout
        printk("SE, %u, %f, %f, %f, %f, %f\n", packet.timestamp, (double) packet.kalmanState.estAltitude, (double) packet.kalmanState.estVelocity, (double) packet.kalmanState.estAcceleration, (double) packet.kalmanState.estBias, (double)packet.effort);

        NPreBoost::SubmitPreBoostPacket(packet);
        k_timer_status_sync(&measurement_timer);
    }
    LOG_INF("Boost Detected");

    // behind schedule bc of boost detect lag
    uint32_t liftoffTimeMs = packet.timestamp - (NUM_SAMPLES_OVER_BOOST_THRESHOLD_REQUIRED * 10'000);
    NTypes::GyroscopeData bias = NPreBoost::GetGyroBias();

    EnableServo();

    LOG_INF("Gyro Bias Estimate: %f %f %f", (double) bias.X, (double) bias.Y, (double) bias.Z);

    // normal flight time
    bool wentOutOfBounds = false;
    for (uint32_t i = 0; i < NUM_FLIGHT_PACKETS; i++) {
        packet.timestamp = packet_timestamp();

        NSensing::MeasureSensors(packet.tempRaw, packet.pressureRaw, packet.accelRaw, packet.gyro);
        float altMeters = NModel::AltitudeMetersFromPressureKPa(packet.pressureRaw) - offset;

        NModel::FeedGyro(packet.timestamp, packet.gyro);
        if (NModel::GyroOutOfBounds()) { // todo maybe make this NModel::GyroEverWentOutOfBounds and have the bool live over there
            wentOutOfBounds = true;
        }

        NModel::FeedKalman(packet.timestamp, altMeters, packet.accelRaw);
        packet.kalmanState = NModel::LastKalmanState();

        packet.effort = NModel::CalcActuatorEffort(packet.kalmanState.estAltitude, packet.kalmanState.estVelocity);
        printk("SE, %u, %f, %f, %f, %f, %f\n", packet.timestamp, (double) packet.kalmanState.estAltitude, (double) packet.kalmanState.estVelocity, (double) packet.kalmanState.estAcceleration, (double) packet.kalmanState.estBias, (double)packet.effort);

        if (packet.timestamp > (liftoffTimeMs + LOCKOUT_MS)) {

            if (wentOutOfBounds) {
                SetServoEffort(0);
            } else {
                SetServoEffort(packet.effort);
            }
        }

        k_timer_status_sync(&measurement_timer);
    }
    LOG_INF("Flight over");
}
