#include "fast_sensing.h"

#include "boost.h"
#include "common.h"
#include "gorbfs.h"

#include <cmath>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
LOG_MODULE_REGISTER(sensing);

K_TIMER_DEFINE(imutimer, NULL, NULL);
K_TIMER_DEFINE(slowdata_timer, NULL, NULL);

static int set_sampling(const struct device *imu_dev);

bool DONT_STOP = true;

int boost_and_flight_sensing(const struct device *superfast_storage, const struct device *imu_dev,
                             const struct device *barom_dev, const struct device *ina_servo,
                             FreakFlightController *freak_controller) {
    set_sampling(imu_dev);

    int64_t start = k_uptime_get();


    k_timer_start(&imutimer, K_MSEC(10), K_MSEC(10));
    FlightState flight_state = FlightState::NotSet;
    bool already_imu_boosted = false;

    static constexpr size_t packets_per_slab = 256/32;
    int ret = 0;

    freak_controller->SubmitEvent(Sources::LSM6DSL, Events::PadReady);
    freak_controller->SubmitEvent(Sources::BMP390, Events::PadReady);

    freak_controller->WaitUntilEvent(Events::PadReady);
    flight_state = FlightState::OnPad;

    while (DONT_STOP && !freak_controller->HasEventOccurred(Events::GroundHit)) {
        for (size_t i = 0; i < packets_per_slab; i++){

        }
    }

    return 0;
}

static int set_sampling(const struct device *imu_dev) {
    struct sensor_value sampling = {0};
    sensor_value_from_float(&sampling, 1666);
    int ret = sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling);
    if (ret < 0) {
        LOG_ERR("Couldnt set sampling\n");
    }
    ret = sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling);
    if (ret < 0) {
        LOG_ERR("Couldnt set sampling\n");
    }
    return 0;
}

int cmd_stop(const struct shell *shell, size_t argc, char **argv) {
    DONT_STOP = false;
    shell_print(shell, "Stopping");
    return 0;
}
