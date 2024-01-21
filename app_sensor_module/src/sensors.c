#include <zephyr/drivers/sensor.h>
#include <zephyr/zephyr.h>
#include <zephyr/logging/log.h>
#include "sensors.h"


LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

SENSOR_MODULE_DATA_T data = {0};


//
// void update_sensor_data(void *dev_arg, void *args, void *process_float_arg) {
//     SENSOR_READINGS_ARGS_T *sensor_args = (SENSOR_READINGS_ARGS_T*) args;
//     
//     int num_readings = sensor_args->num_readings;
//     int process_float = POINTER_TO_INT(process_float_arg);
//     
//     struct device *dev = (struct device *) dev_arg;
//     enum sensor_channel *channels = sensor_args->channels;
//     
//     struct sensor_value **values = sensor_args->values;
//     float **float_values = sensor_args->float_values;
//     
//     while (1) {
//         if (sensor_sample_fetch(dev)) {
//              LOG_ERR("Failed to update %s readings\n", dev->name);
//              continue;
//         }
//
//         for (int i = 0; i < num_readings; i++) {
//             if (sensor_channel_get(dev, channels[i], values[i])) {
//                 LOG_ERR("Failed to get sensor channel\n"); // TODO: Be more specific
//             }
//
//             if (process_float) {
//                 *float_values[i] = sensor_value_to_float(values[i]);
//             }
//         }
//     }
// }

