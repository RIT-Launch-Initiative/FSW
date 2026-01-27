#include "n_model.h"
#include "n_storage.h"
#include "servo.h"

#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_AIRBRAKE_LOG_LEVEL);

SYS_INIT(servo_init, APPLICATION, 1);
SYS_INIT(storage_init, APPLICATION, 2);

int main() {
    k_msleep(1000);

    printk("thinking\n");
    uint64_t uptimeUS = k_ticks_to_us_near64(k_uptime_ticks());
    float alt_m = 1.0;
    float accel_v = 16.8;

    uint64_t start = k_cycle_get_64();

    NModel::FeedKalman(uptimeUS, alt_m, accel_v);
    uint64_t postUpdate = k_cycle_get_64();
    NModel::KalmanState state = NModel::LastKalmanState();
    uint64_t postState = k_cycle_get_64();
    float effort = NModel::CalcActuatorEffort(0.F, 1.F);
    uint64_t postEffort = k_cycle_get_64();

    uint64_t fullDelta = k_cycle_get_64() - start;
    uint64_t updateDelta = postUpdate - start;
    uint64_t getDelta = postState - postUpdate;
    uint64_t effortDelta = postEffort - postState;


    k_msleep(1000);

    printk("Time to break air\n");
    printk("Alt Meas: %f, Acc Meas: %.f\n", (double) alt_m, (double) accel_v);
    printk("Alt Est: %f, Vel Est: %f, Acc Est: %.f\n", (double) state.estAltitude, (double) state.estVelocity,
           (double) state.estAcceleration);
    printk("Effort: %f\n", (double) effort);
    printk("=====================================\n");
    printk("Update: %lld cycles (%d us)\n", updateDelta, k_cyc_to_us_near32(updateDelta));
    printk("Get:    %lld cycles (%d us)\n", getDelta, k_cyc_to_us_near32(getDelta));
    printk("LUT:    %lld cycles (%d us)\n", effortDelta, k_cyc_to_us_near32(effortDelta));
    printk("Total:  %lld cycles (%d us)\n", fullDelta, k_cyc_to_us_near32(fullDelta));

}
