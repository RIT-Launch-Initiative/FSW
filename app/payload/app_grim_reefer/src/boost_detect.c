#include "boost_detect.h"

#include "buzzer.h"
#include "config.h"
#include "data_storage.h"

#include <zephyr/fs/fs.h>
// Launch Core Includes
#include <launch_core_classic/conversions.h>
#include <launch_core_classic/dev/dev_common.h>
#include <launch_core_classic/types.h>

// Zephyr Includes
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(boost_detect, CONFIG_APP_GRIM_REEFER_LOG_LEVEL);

// Forward Declares
static void altitude_boost_reading_task(void);
static void accel_boost_reading_task(void);
static void battery_read_task(void);

// Timers
K_TIMER_DEFINE(altitude_boost_detect_timer, NULL, NULL);
K_TIMER_DEFINE(accel_boost_detect_timer, NULL, NULL);
K_TIMER_DEFINE(battery_timer, NULL, NULL);

// Events
#define BEGIN_BOOST_DETECT_EVENT 2
K_EVENT_DEFINE(begin_boost_detect);

// Threads
K_THREAD_DEFINE(altimeter_boost_thread, 1024, altitude_boost_reading_task, NULL, NULL, NULL, BOOST_DETECT_ALT_PRIORITY,
                0, THREAD_START_DELAY);
K_THREAD_DEFINE(accel_boost_thread, 1024, accel_boost_reading_task, NULL, NULL, NULL, BOOST_DETECT_IMU_PRIORITY, 0,
                THREAD_START_DELAY);
K_THREAD_DEFINE(battery_thread, 1024, battery_read_task, NULL, NULL, NULL, BOOST_DETECT_IMU_PRIORITY, 0,
                THREAD_START_DELAY);

volatile bool boost_detected = false;
extern bool flight_cancelled;
// Rolling buffers

struct fast_data accel_buffer[ACCEL_BUFFER_SIZE] = {0};
int accel_buffer_index = 0;
float accel_running_sum = {0.0};

struct altitude_data {
    uint32_t timestamp;
    l_barometer_data_t barom;
};

struct altitude_data altitude_buffer[ALTITUDE_BUFFER_SIZE] = {0};
int altitude_buffer_index;

static int read_channel_to_float(const struct device* dev, enum sensor_channel chan, float* fval) {
    struct sensor_value val = {0};
    int ret = sensor_channel_get(dev, chan, &val);
    float v = sensor_value_to_float(&val);
    *fval = v;
    return ret;
}
double l_altitude_conversion(double pressure_kpa, double temperature_c) {
    double pressure = pressure_kpa * 10;
    double altitude = (1 - pow(pressure / 1013.25, 0.190284)) * 145366.45 * 0.3048;
    return altitude;
}

static void altitude_boost_reading_task(void) {
    struct altitude_data data = {0};

    k_event_wait(&begin_boost_detect, BEGIN_BOOST_DETECT_EVENT, false, K_FOREVER);
    while (true) {
        k_timer_status_sync(&altitude_boost_detect_timer);
        if (boost_detected || flight_cancelled) {
            break;
        }
        const struct device* altimeter_dev = (const struct device*) k_timer_user_data_get(&altitude_boost_detect_timer);

        if (altimeter_dev == NULL) {
            LOG_ERR("No altitude device given to detect launch with, %p", altimeter_dev);
            continue;
        }
        // Read Sensor
        sensor_sample_fetch(altimeter_dev);
        float press = 0;
        float temp = 0;
        read_channel_to_float(altimeter_dev, SENSOR_CHAN_PRESS, &press);
        read_channel_to_float(altimeter_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        data.barom.pressure = press;
        data.barom.temperature = temp;
        data.timestamp = k_uptime_get();
        // Write to buffer
        struct altitude_data oldest = altitude_buffer[altitude_buffer_index];
        altitude_buffer[altitude_buffer_index] = data;
        altitude_buffer_index = (altitude_buffer_index + 1) % ARRAY_SIZE(altitude_buffer);
        // Do altitude calcs
        if (oldest.timestamp == 0) {
            // these checks are invalid if the comparison value is uninitialized
            continue;
        }
        double old_alt = l_altitude_conversion(oldest.barom.pressure, oldest.barom.temperature);
        double alt = l_altitude_conversion(press, temp);
        if (fabs(alt - old_alt) > ALTITUDE_VAL_THRESHOLD) {
            LOG_INF("Altitude Boost Detect");
            boost_detected = true;
            break;
        }
    }
}

#define MAG_SQRED(v) (v.accel_x * v.accel_x + v.accel_y * v.accel_y + v.accel_z * v.accel_z)

static void accel_boost_reading_task(void) {

    struct fast_data data = {0};

    k_event_wait(&begin_boost_detect, BEGIN_BOOST_DETECT_EVENT, false, K_FOREVER);
    while (true) {
        k_timer_status_sync(&accel_boost_detect_timer);
        if (boost_detected || flight_cancelled) {
            break;
        }
        const struct device* imu_dev = (const struct device*) k_timer_user_data_get(&accel_boost_detect_timer);

        if (imu_dev == NULL) {
            LOG_ERR("No IMU device given to detect launch with, %p", imu_dev);
            continue;
        }
        // Read sensor
        sensor_sample_fetch(imu_dev);
        float x, y, z = 0;
        read_channel_to_float(imu_dev, SENSOR_CHAN_ACCEL_X, &x);
        read_channel_to_float(imu_dev, SENSOR_CHAN_ACCEL_Y, &y);
        read_channel_to_float(imu_dev, SENSOR_CHAN_ACCEL_Z, &z);
        data.acc.accel_x = x;
        data.acc.accel_y = y;
        data.acc.accel_z = z;

        read_channel_to_float(imu_dev, SENSOR_CHAN_GYRO_X, &x);
        read_channel_to_float(imu_dev, SENSOR_CHAN_GYRO_Y, &y);
        read_channel_to_float(imu_dev, SENSOR_CHAN_GYRO_Z, &z);
        data.gyro.gyro_x = x;
        data.gyro.gyro_y = y;
        data.gyro.gyro_z = z;

        data.timestamp = k_uptime_get();
        // Add to circular buffer
        struct fast_data oldest = accel_buffer[accel_buffer_index];
        accel_buffer[accel_buffer_index] = data;
        accel_buffer_index = (accel_buffer_index + 1) % ARRAY_SIZE(accel_buffer);

        // Do acceleration calcs

#ifdef IMU_BOOST_DETECTION_MODE_AXIS
        float reading = data.acc.IMU_UP_AXIS;
        float last_reading = oldest.acc.IMU_UP_AXIS;
        accel_running_sum -= last_reading;
        accel_running_sum += reading;
        float avg = accel_running_sum / ACCEL_BUFFER_SIZE;
        if (avg > ACCEL_VAL_THRESHOLD) {
            LOG_INF("Accel Boost Detect");
            boost_detected = true;
            break;
        }
#else
        float reading = MAG_SQRED(data.acc);
        float last_reading = MAG_SQRED(oldest.acc);
        accel_running_sum -= last_reading;
        accel_running_sum += reading;

        float avg = accel_running_sum / ACCEL_BUFFER_SIZE;
        if (avg > ACCEL_VAL_THRESHOLD * ACCEL_VAL_THRESHOLD) {
            LOG_INF("Accel Boost Detect");
            boost_detected = true;
            break;
        }
#endif
    }
}

static void battery_read_task(void) {
    k_event_wait(&begin_boost_detect, BEGIN_BOOST_DETECT_EVENT, false, K_FOREVER);
    const struct device* ina = k_timer_user_data_get(&battery_timer);
    int count_below = 0;
    while (!flight_cancelled && !boost_detected) {
        k_timer_status_sync(&battery_timer);
        sensor_sample_fetch(ina);
        float voltage = 0;
        read_channel_to_float(ina, SENSOR_CHAN_VOLTAGE, &voltage);

        if (voltage < (float) LOW_BATTERY_VOLTAGE) {
            count_below++;
        }

        if (count_below > 4) {
            buzzer_tell(buzzer_cond_low_battery);
            break;
        }
    }
}

void start_boost_detect(const struct device* imu, const struct device* altimeter, const struct device* battery_ina) {
    LOG_INF("Starting Boost Detection: %p, %p", imu, altimeter);

    k_timer_user_data_set(&accel_boost_detect_timer, (void*) imu);
    k_timer_user_data_set(&altitude_boost_detect_timer, (void*) altimeter);
    k_timer_user_data_set(&battery_timer, (void*) battery_ina);

    k_timer_start(&accel_boost_detect_timer, FAST_DATA_DELAY, FAST_DATA_DELAY);
    k_timer_start(&altitude_boost_detect_timer, ALTIM_DATA_DELAY, ALTIM_DATA_DELAY);
    k_timer_start(&battery_timer, K_SECONDS(1), K_SECONDS(1));

    k_event_set(&begin_boost_detect, BEGIN_BOOST_DETECT_EVENT);
}

void stop_boost_detect() {
    LOG_INF("Ending Boost Detection");
    k_timer_stop(&altitude_boost_detect_timer);
    k_timer_stop(&accel_boost_detect_timer);
    k_timer_stop(&battery_timer);
}

bool get_boost_detected() { return boost_detected; }

void save_full_buffer(uint8_t* data, size_t length, const char* filename) {
    struct fs_file_t file;
    fs_file_t_init(&file);
    int ret = fs_open(&file, filename, FS_O_WRITE | FS_O_CREATE);

    if (ret < 0) {
        LOG_ERR("Error opening %s. %d", filename, ret);
        return;
    }
    ret = fs_write(&file, data, length);
    if (ret < 0) {
        LOG_INF("Error writing %s", filename);
    }
    ret = fs_close(&file);
    if (ret < 0) {
        LOG_ERR("Failed to save adc file. %d", ret);
    }
    LOG_INF("Saved %s", filename);
}

void save_boost_data() {
    save_full_buffer((uint8_t*) accel_buffer, sizeof(accel_buffer), PRELAUNCH_ACCEL_FILENAME);
    save_full_buffer((uint8_t*) altitude_buffer, sizeof(altitude_buffer), PRELAUNCH_ALT_FILENAME);
}