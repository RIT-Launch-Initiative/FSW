#include "math/matrix.hpp"
#include "n_boost.hpp"
#include "n_buzzer.hpp"
#include "n_model.hpp"
#include "n_preboost.hpp"
#include "n_sensing.hpp"
#include "n_storage.hpp"
#include "servo.hpp"

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <cmath>

#include <zsl/orientation/quaternions.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_AIRBRAKE_LOG_LEVEL);

SYS_INIT(servo_init, APPLICATION, 1);
SYS_INIT(storage_init, APPLICATION, 2);
SYS_INIT(buzzer_init, APPLICATION, 2);

K_TIMER_DEFINE(measurement_timer, NULL, NULL);

uint32_t packet_timestamp() {
    int64_t ms = k_uptime_get();
    return (uint32_t) ms;
}

// helper to return from main early if the main mission is cancelled via shell
#define RETURN0_IF_CANCELLED                                                                                           \
    if (IsFlightCancelled()) {                                                                                         \
        return 0;                                                                                                      \
    }

NTypes::GyroscopeData unbiasGyro(const NTypes::GyroscopeData &data, const NTypes::GyroscopeData &bias){
    return {
        .X = data.X - bias.X,
        .Y = data.Y - bias.Y,
        .Z = data.Z - bias.Z
    };
}

int main() {
    NSensing::InitSensors();

    if (NStorage::HasStoredFlight()) {
        CancelFlight();
        LOG_WRN("NOT FLYING");
        NBuzzer::NotFlying();
        return 0;
    }

    k_timer_start(&measurement_timer, K_MSEC(10), K_MSEC(10));

    Packet packet{
        .timestamp = 0,
        .tempRaw = 0,
        .pressureRaw = 0,
        .accelRaw = 0,
        .gyro = {0},
        .kalmanState = {0},
        .kalmanInnovation = {0, 0},
        .orientationMatrix = {1, 0, 0, 0, 1, 0, 0, 0, 1},
        .effort = 0,
    };

    Parameters params{};
    params.bootcount = NStorage::GetBootcount();
    NSensing::MeasureSensors(packet.tempRaw, packet.pressureRaw, packet.accelRaw, packet.gyro);
    float vertical = GetUpAxis(packet.accelRaw);

    NBoost::FeedDetector(vertical);

    // submit initial preboost packet (will almost certainly be overwritten but we want to get everything going before feeding the filter)
    NPreBoost::SubmitPreBoostPacket(packet);

    // servo not allowed until after under mach. disable to save power
    DisableServo();
    while (!NBoost::IsDetected()) {
        RETURN0_IF_CANCELLED;
        k_timer_status_sync(&measurement_timer);
        packet.timestamp = packet_timestamp();
        NSensing::MeasureSensors(packet.tempRaw, packet.pressureRaw, packet.accelRaw, packet.gyro);
        
        float vertical = GetUpAxis(packet.accelRaw);

        NBoost::FeedDetector(vertical);

        float altMeters = NModel::AltitudeMetersFromPressureKPa(packet.pressureRaw) - NPreBoost::GetGroundLevelASL();
        NModel::FeedKalman(packet.timestamp, altMeters, vertical);

        packet.kalmanState = NModel::LastKalmanState();
        packet.effort = 0; // no fun until after burnout

        NPreBoost::SubmitPreBoostPacket(packet);
    }
    RETURN0_IF_CANCELLED;
    LOG_INF("Boost Detected");
    // NBuzzer::SetBuzzer(true);

    // behind schedule bc of boost detect lag
    uint32_t liftoffTimeMs = packet.timestamp - (NUM_SAMPLES_OVER_BOOST_THRESHOLD_REQUIRED * 10);
    NTypes::GyroscopeData bias = NPreBoost::GetGyroBias();
    float groundLevelASLMeters = NPreBoost::GetGroundLevelASL();

    params.timestampOfBoostDetect = packet.timestamp;
    params.gyroBias = bias;
    params.preBoostPressure = NPreBoost::GetGroundLevelPressure();
    NStorage::WriteParameters(&params);
    EnableServo();

    LOG_INF("Gyro Bias Estimate: %f %f %f", (double) bias.X, (double) bias.Y, (double) bias.Z);
    uint32_t preboostWriteHead = 0;

    // normal flight time
    for (uint32_t i = 0; i < NUM_FLIGHT_PACKETS; i++) {
        RETURN0_IF_CANCELLED;
        k_timer_status_sync(&measurement_timer);

        packet.timestamp = packet_timestamp();

        NSensing::MeasureSensors(packet.tempRaw, packet.pressureRaw, packet.accelRaw, packet.gyro);
        float altMeters = NModel::AltitudeMetersFromPressureKPa(packet.pressureRaw) - groundLevelASLMeters;
        float vertical = GetUpAxis(packet.accelRaw);


        NTypes::GyroscopeData unbiasedGyro = unbiasGyro(packet.gyro, bias);
        NModel::FeedGyro(packet.timestamp, unbiasedGyro);
        NModel::FillPacketWithKalmanInnovation(packet.kalmanInnovation);
        NModel::FillPacketWithOrientationMatrix(packet.orientationMatrix);

        NModel::FeedKalman(packet.timestamp, altMeters, vertical);
        packet.kalmanState = NModel::LastKalmanState();

        packet.effort = NModel::CalcActuatorEffort(packet.kalmanState.estAltitude, packet.kalmanState.estVelocity);

        if (packet.timestamp > (liftoffTimeMs + LOCKOUT_MS)) {
            if (NModel::EverWentOutOfBounds()) {
                SetServoEffort(0);
            } else {
                SetServoEffort(packet.effort);
            }
        }

        NStorage::WriteFlightPacket(i, &packet);



        // Write preboost if needed
        if (preboostWriteHead < NUM_STORED_PREBOOST_PACKETS) {
            NStorage::WritePreboostPacket(preboostWriteHead, NPreBoost::GetPreBoostPacketPtr(preboostWriteHead));
            preboostWriteHead++;
        } else {
            // NBuzzer::SetBuzzer(false);
        }
    }
    LOG_INF("Flight over");
    DisableServo();
    CancelFlight();

    // happy beep for reco team
    while(true){
        NBuzzer::SetBuzzer(true);
        k_msleep(200);
        NBuzzer::SetBuzzer(false);
        k_msleep(50);
        NBuzzer::SetBuzzer(true);
        k_msleep(100);
        NBuzzer::SetBuzzer(false);
        k_msleep(50);
    }
}

static atomic_t flightCancelled = ATOMIC_INIT(0);
void CancelFlight() {
    atomic_t prev = atomic_set(&flightCancelled, ATOMIC_INIT(1));
    if (prev != ATOMIC_INIT(1)) {
        LOG_INF("Cancelled flight");
    }
}
bool IsFlightCancelled() { return atomic_get(&flightCancelled) == 1; }


float GetUpAxis(const NTypes::AccelerometerData &xyz){
    NTypes::AccelerometerData out{0,0,0};
    RotateIMUVectorToRocketVector(xyz, out);
    return out.Z;
}
void RotateIMUVectorToRocketVector(const NTypes::AccelerometerData &xyz, NTypes::AccelerometerData &out){
    zsl_quat p{.r = 0, .i = xyz.X, .j = xyz.Y, .k = xyz.Z};

    // q p q*
    zsl_quat intermediate;
    zsl_quat_mult(&IMU_TO_ROCKET_QUAT, &p, &intermediate);

    zsl_quat output;
    zsl_quat_mult(&intermediate, &IMU_TO_ROCKET_QUAT_CONJUGATE, &output);
    out.X = output.i;
    out.Y = output.j;
    out.Z = output.k;
}

void RotateRocketVectorToIMUVector(const NTypes::AccelerometerData &xyz, NTypes::AccelerometerData &out){
    zsl_quat p{.r = 0, .i = xyz.X, .j = xyz.Y, .k = xyz.Z};

    // q p q*
    zsl_quat intermediate;
    zsl_quat_mult(&IMU_TO_ROCKET_QUAT_CONJUGATE, &p, &intermediate);

    zsl_quat output;
    zsl_quat_mult(&intermediate, &IMU_TO_ROCKET_QUAT, &output);
    out.X = output.i;
    out.Y = output.j;
    out.Z = output.k;
}