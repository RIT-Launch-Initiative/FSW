#include "fast_sensing.h"

#include "boost.h"
#include "common.h"
#include "gorbfs.h"

#include <cmath>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
LOG_MODULE_REGISTER(sensing);

const struct device *ina_dev = DEVICE_DT_GET(DT_ALIAS(ina_servo));
const struct device *imu_dev = DEVICE_DT_GET(DT_ALIAS(imu));
const struct device *barom_dev = DEVICE_DT_GET(DT_ALIAS(barom));

struct Measurements {
    float current;
    float voltage;
    float pressure;
    float temperature;
    float axis_accel;
};

int measure_all(Measurements *measurements) {
    int ina_ret = read_ina(ina_dev, measurements->voltage, measurements->current);
    int lsm_ret = read_imu_up(imu_dev, measurements->axis_accel);
    int bmp_ret = read_barom(barom_dev, measurements->temperature, measurements->pressure);
    if (ina_ret != 0) {
        LOG_WRN("Couldnt read ina: %d", ina_ret);
        return ina_ret;
    }
    if (lsm_ret != 0) {
        LOG_WRN("Couldnt read lsm: %d", lsm_ret);
        return lsm_ret;
    }
    if (bmp_ret != 0) {
        LOG_WRN("Couldnt read lsm: %d", bmp_ret);
        return bmp_ret;
    }
    return 0;
}

int boost_and_flight_sensing(const struct device *superfast_storage, const struct device *imu_dev,
                             const struct device *barom_dev, const struct device *ina_servo,
                             FreakFlightController *freak_controller) {
    set_lsm_sampling(imu_dev, 1666);

    FlightState flight_state = FlightState::NotSet;
    bool already_imu_boosted = false;

    freak_controller->SubmitEvent(Sources::LSM6DSL, Events::PadReady);
    freak_controller->SubmitEvent(Sources::BMP390, Events::PadReady);

    freak_controller->WaitUntilEvent(Events::PadReady);
    flight_state = FlightState::OnPad;

    while (!freak_controller->HasEventOccurred(Events::GroundHit)) {
        NTypes::FastPacket *packet = NULL;
        for (size_t i = 0; i < FAST_PACKET_ITEMS_PER_PACKET; i++) {
            NTypes::FastPacketItem *item = &packet->items[i];
        }
    }

    return 0;
}
