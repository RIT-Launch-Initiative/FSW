#ifndef REEFER_INLCUDE_DATA_READING_H
#define REEFER_INLCUDE_DATA_READING_H

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>

struct fast_devices {
    const struct device* imu;
    const struct device* altim;
};
struct slow_devices {
    const struct device* altim;
    const struct device* ina_bat;
    const struct device* ina_grim;
    const struct device* ina_adc;
};

struct data_devices {
    struct fast_devices fast;
    struct slow_devices slow;
    const struct adc_dt_spec* chan;
};

/**
 * start data reading
 * Tell the system to begin measuring data
 * @param devs the devices needed
 * @param altimeter the altimeter device to watch for a fast change in height
*/
void start_data_reading(const struct data_devices* devs);
/**
 * Tell the system to stop reading data
 */
void stop_data_reading();

#endif