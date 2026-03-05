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

volatile float zro = 0;
volatile float one = 1;
volatile float two = 2;
volatile float thr = 3;

void print3x3(const Matrix<3, 3> &mat) {
    printk("%+.4f  %+.4f  %+.4f\n", (double) mat.Get(0, 0), (double) mat.Get(0, 1), (double) mat.Get(0, 2));
    printk("%+.4f  %+.4f  %+.4f\n", (double) mat.Get(1, 0), (double) mat.Get(1, 1), (double) mat.Get(1, 2));
    printk("%+.4f  %+.4f  %+.4f\n", (double) mat.Get(2, 0), (double) mat.Get(2, 1), (double) mat.Get(2, 2));
}
// 
int main() {
    // NBuzzer::SetBuzzer(true);
    NSensing::InitSensors();

    const float arr[] = {zro, one, two, one, zro, thr, two, thr, zro};

    Matrix<3, 3> At{arr};
    Matrix<3, 3> res = matrixExp(At, 15);

    printk("At:\n");
    print3x3(At);
    printk("e^At:\n");
    print3x3(res);

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
        .orientationQuat = {1, 0, 0, 0},
        .effort = 0,
    };

    Parameters params{};
    params.bootcount = NStorage::GetBootcount();
    NSensing::MeasureSensors(packet.tempRaw, packet.pressureRaw, packet.accelRaw, packet.gyro);
    float vertical = UpAxisFrom(UP_AXIS, packet.accelRaw);

    NBoost::FeedDetector(vertical);

    // submit initial preboost packet (will almost certainly be overwritten but we want to get everything going before feeding the filter)
    NPreBoost::SubmitPreBoostPacket(packet);

    // servo not allowed until after under mach. disable to save power
    DisableServo();
    int64_t elapsed_total_sum = 0;
    int64_t elapsed_calc_sum = 0;
    int i = 0;
    while (!NBoost::IsDetected()) {
        RETURN0_IF_CANCELLED;
        k_timer_status_sync(&measurement_timer);
        int64_t total_start = k_cycle_get_64();
        packet.timestamp = packet_timestamp();
        NSensing::MeasureSensors(packet.tempRaw, packet.pressureRaw, packet.accelRaw, packet.gyro);
        int64_t think_start = k_cycle_get_64();
        float vertical = UpAxisFrom(UP_AXIS, packet.accelRaw);

        NBoost::FeedDetector(vertical);

        float altMeters = NModel::AltitudeMetersFromPressureKPa(packet.pressureRaw) - NPreBoost::GetGroundLevelASL();
        NModel::FeedKalman(packet.timestamp, altMeters, vertical);

        packet.kalmanState = NModel::LastKalmanState();
        packet.effort = 0; // no fun until after burnout

        NPreBoost::SubmitPreBoostPacket(packet);
        int64_t end = k_cycle_get_64();
        int64_t total_elapsed = end - total_start;
        int64_t think_elapsed = end - think_start;
        int total_us_elapsed = k_cyc_to_us_near32(total_elapsed);
        int think_us_elapsed = k_cyc_to_us_near32(think_elapsed);
        elapsed_total_sum += total_us_elapsed;
        elapsed_calc_sum += think_us_elapsed;
        i++;
        if (i % 20 == 0) {
            // LOG_INF("Took %d - %d for cals", (int)(elapsed_total_sum / 20), (int)(elapsed_calc_sum/20));
            elapsed_total_sum = 0;
            elapsed_calc_sum = 0;
        }
    }
    RETURN0_IF_CANCELLED;
    LOG_INF("Boost Detected");

    // behind schedule bc of boost detect lag
    uint32_t liftoffTimeMs = packet.timestamp - (NUM_SAMPLES_OVER_BOOST_THRESHOLD_REQUIRED * 10);
    NTypes::GyroscopeData bias = NPreBoost::GetGyroBias();
    float groundLevelASLMeters = NPreBoost::GetGroundLevelASL();

    params.timestampOfBoost = packet.timestamp;
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
        float vertical = UpAxisFrom(UP_AXIS, packet.accelRaw);

        NModel::FeedGyro(packet.timestamp, packet.gyro);

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
        }
    }
    LOG_INF("Flight over");
    DisableServo();
}

static atomic_t flightCancelled = ATOMIC_INIT(0);
void CancelFlight() {
    atomic_t prev = atomic_set(&flightCancelled, ATOMIC_INIT(1));
    if (prev != ATOMIC_INIT(1)) {
        LOG_INF("Cancelled flight");
    }
}
bool IsFlightCancelled() { return atomic_get(&flightCancelled) == 1; }