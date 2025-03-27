/*
 * Copyright (c) 2023 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "boost_detect.h"
#include "buzzer.h"
#include "config.h"
#include "data_reading.h"
#include "data_storage.h"
#include "zephyr/toolchain.h"

#include <math.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/smf.h>


LOG_MODULE_REGISTER(main, CONFIG_APP_GRIM_REEFER_LOG_LEVEL);

// Device Tree Gets
#define LED1_NODE DT_NODELABEL(led1)
#define LED2_NODE DT_NODELABEL(led2)

const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);

#define LDO_EN_NODE DT_NODELABEL(ldo_enable)
#define CAM_EN_NODE DT_NODELABEL(cam_enable)

const struct gpio_dt_spec ldo_enable = GPIO_DT_SPEC_GET(LDO_EN_NODE, gpios);
const struct gpio_dt_spec cam_enable = GPIO_DT_SPEC_GET(CAM_EN_NODE, gpios);

#define BUZZER_NODE DT_NODELABEL(buzzer)
const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(BUZZER_NODE, gpios);

#define DBG_SERIAL_NODE DT_ALIAS(debug_serial)
const struct device *const debug_serial_dev = DEVICE_DT_GET(DBG_SERIAL_NODE);

#define BME280_NODE DT_NODELABEL(bme280)
const struct device *bme280_dev = DEVICE_DT_GET(BME280_NODE);

#define LSM6DSL_NODE DT_NODELABEL(lsm6dsl)
const struct device *lsm6dsl_dev = DEVICE_DT_GET(LSM6DSL_NODE);

#define FLASH_NODE DT_NODELABEL(w25q512)
const struct device *flash_dev = DEVICE_DT_GET(FLASH_NODE);

// Inas
#define INA_BAT_NODE DT_NODELABEL(ina260_battery)
const struct device *ina_bat_dev = DEVICE_DT_GET(INA_BAT_NODE);

#define INA_LDO_NODE DT_NODELABEL(ina260_ldo)
const struct device *ina_ldo_dev = DEVICE_DT_GET(INA_LDO_NODE);

#define INA_GRIM_NODE DT_NODELABEL(ina260_3v3)
const struct device *ina_grim_dev = DEVICE_DT_GET(INA_GRIM_NODE);

static const struct adc_dt_spec adc_chan0 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

// Cannnot be statically initialized :(
struct data_devices data_devices = {0};

/**
 * Check if a gpio is ready or return a status code indicating that its not.
 * gpio: gpio_dt_spec
 * name: a string describing this pin. gpio_dt_spec only knows the name of its gpio controller device so make this a friendlier name
*/
#define CHECK_GPIO_OUTPUT(gpio, name)                                                                                  \
    if (!gpio_is_ready_dt(&gpio)) {                                                                                    \
        LOG_ERR("%s is not ready", name);                                                                              \
        return -ENODEV;                                                                                                \
    }                                                                                                                  \
    if (gpio_pin_configure_dt(&gpio, GPIO_OUTPUT_ACTIVE) < 0) {                                                        \
        LOG_ERR("Unable to configure %s output pin\n", name);                                                          \
        return -ENODEV;                                                                                                \
    }

/**
 * Check if a given device is ready. Returning -ENODEV if it's not
*/
#define CHECK_DEVICE_READY(dev)                                                                                        \
    if (!device_is_ready(dev)) {                                                                                       \
        LOG_ERR("Device %s is not ready.", dev->name);                                                                 \
        return -ENODEV;                                                                                                \
    }

/**
 * @brief Setup all the gpio pins needed for flight
 * @return -ENODEV if a GPIO couldnt be setup. 0 otherwise
 */
static int gpio_init(void) {
    // Init LEDS
    CHECK_GPIO_OUTPUT(led1, "led1");
    CHECK_GPIO_OUTPUT(led2, "led2");
    CHECK_GPIO_OUTPUT(ldo_enable, "ldo_enable");
    CHECK_GPIO_OUTPUT(cam_enable, "cam_enable");
    CHECK_GPIO_OUTPUT(buzzer, "buzzer");

    gpio_pin_set_dt(&led1, 0);
    gpio_pin_set_dt(&led2, 0);
    gpio_pin_set_dt(&ldo_enable, 1);
    gpio_pin_set_dt(&cam_enable, 0);
    gpio_pin_set_dt(&buzzer, 0);

    return 0;
}
/**
 * @brief Initialize flash and all sensors. Flash, bme280, adc, and inas
 * @return -EIO if flash is not found. -ENODEV if any sensor isn't ready. 0 if all ok 
 */
static int sensor_init(void) {
    const bool flash_found = device_is_ready(flash_dev);
    if (!flash_found) {
        return -EIO;
    }
    CHECK_DEVICE_READY(lsm6dsl_dev)
    CHECK_DEVICE_READY(bme280_dev)

    CHECK_DEVICE_READY(ina_bat_dev)
    CHECK_DEVICE_READY(ina_ldo_dev)
    CHECK_DEVICE_READY(ina_grim_dev)

    // ADC
    if (!adc_is_ready_dt(&adc_chan0)) {
        LOG_ERR("ADC controller device %s not ready\n", adc_chan0.dev->name);
        return -ENODEV;
    }
    int ret = adc_channel_setup_dt(&adc_chan0);
    if (ret < 0) {
        LOG_ERR("Could not setup ADC channel: %d\n", ret);
        return -ENODEV;
    }

    CHECK_DEVICE_READY(debug_serial_dev)

    return 0;
}

// Flight Phase
K_TIMER_DEFINE(flight_duration_timer, NULL, NULL);
K_TIMER_DEFINE(camera_extra_timer, NULL, NULL);
// State Machine Setup
enum flight_state { PAD_STATE, FLIGHT_STATE, LANDED_STATE };
const struct smf_state flight_states[];
struct {
    struct smf_ctx ctx;
    /* All User Defined Data Follows */
} s_obj;

extern bool flight_over;
bool flight_cancelled = false;

// On Pad, boost detecting
static void pad_state_entry(void *o) {
    LOG_INF("On Pad");
    start_boost_detect(lsm6dsl_dev, bme280_dev, ina_bat_dev);
}
static void pad_state_run(void *o) {
    if (get_boost_detected()) {
        smf_set_state(SMF_CTX(&s_obj), &flight_states[FLIGHT_STATE]);
    }
}
static void pad_state_exit(void *o) { stop_boost_detect(); }

// In flight, data sampling
static void flight_state_entry(void *o) {
    LOG_INF("Flight Started");
    // Turn on Cameras and ADC
    gpio_pin_set_dt(&ldo_enable, 1);
    gpio_pin_set_dt(&cam_enable, 1);
    buzzer_tell(buzzer_cond_launched);
    k_timer_start(&flight_duration_timer, TOTAL_FLIGHT_TIME, K_NO_WAIT);
    start_data_reading(&data_devices);

    save_boost_data();
}
static void flight_state_run(void *o) {
    k_timer_status_sync(&flight_duration_timer);
    smf_set_state(SMF_CTX(&s_obj), &flight_states[LANDED_STATE]);
}
static void flight_state_exit(void *o) {
    flight_over = true;
    stop_data_reading();
}

// Landed, saving files and waiting for cameras
static void landed_state_entry(void *o) {
    // Stop logging, start saving
    LOG_INF("Landed");
    k_timer_start(&camera_extra_timer, CAMERA_EXTRA_TIME, K_NO_WAIT);

    finish_data_storage();
    buzzer_tell(buzzer_cond_landed);
}
static void landed_state_run(void *o) {
    if (k_timer_status_get(&camera_extra_timer) > 0) {
        // timer has expired
        smf_set_terminate(SMF_CTX(&s_obj), 1);
    }
}
static void landed_state_exit(void *o) {}

void describe_flight() {
    printk("Config Parameters:\n");
#ifdef BUZZER_USE_LED
    printk("  Buzzer:                    LED\n");
#else
    printk("  Buzzer:                    Out Loud\n");
#endif
#ifdef SHORT_FLIGHT
    printk("  Flight Mode:               Short\n");
#else
    printk("  Flight Mode:               Full\n");
#endif

    printk("Boost Detect Parameters:\n");
#ifdef IMU_BOOST_DETECTION_MODE_AXIS
    printk("  Mode:                      %s\n", STRINGIFY(IMU_UP_AXIS));
#else
    printk("  Mode:                      Magnitude\n");
#endif
    printk("  Accelaration threshold:    %.2f G\n", (double) ACCEL_VAL_THRESHOLD / 9.81);
    printk("  Altitude threshold:        %.2f ft\n", ((double) ALTITUDE_VAL_THRESHOLD) * 3.28084);

    printk("  Accelaration buffer size:  %d\n", ACCEL_BUFFER_SIZE);
    printk("  Altitude buffer size:      %d\n", ALTITUDE_BUFFER_SIZE);
}

const struct smf_state flight_states[] = {
    [PAD_STATE] = SMF_CREATE_STATE(pad_state_entry, pad_state_run, pad_state_exit),
    [FLIGHT_STATE] = SMF_CREATE_STATE(flight_state_entry, flight_state_run, flight_state_exit),
    [LANDED_STATE] = SMF_CREATE_STATE(landed_state_entry, landed_state_run, landed_state_exit)};

int main(void) {

    if (gpio_init() != 0) {
        LOG_ERR("GPIO not setup. Continuing...\n");
        buzzer_tell(buzzer_cond_missing_sensors);
    }
    int ret = sensor_init();
    if (ret == -EIO) {
        LOG_ERR("Flash is not functional");
        buzzer_tell(buzzer_cond_noflash);
    } else if (ret == -ENODEV) {
        LOG_ERR("Some sensors not functional");
        buzzer_tell(buzzer_cond_missing_sensors);
    }

    describe_flight();

    data_devices.fast.imu = lsm6dsl_dev;
    data_devices.fast.altim = bme280_dev;
    data_devices.slow.altim = bme280_dev;
    data_devices.slow.ina_adc = ina_ldo_dev;
    data_devices.slow.ina_bat = ina_bat_dev;
    data_devices.slow.ina_grim = ina_grim_dev;
    data_devices.chan = &adc_chan0;

#ifdef BUZZER_USE_LED
    begin_buzzer_thread(&led1);
#else
    begin_buzzer_thread(&buzzer);
#endif

    if (start_data_storage_thread() != 0) {
        LOG_ERR("Failed to initialize data storage. FATAL ERROR\n");
        buzzer_tell(buzzer_cond_noflash);
    }

    // Begin State machine
    smf_set_initial(SMF_CTX(&s_obj), &flight_states[PAD_STATE]);
    while (1) {
        /* State machine terminates if a non-zero value is returned */
        if (flight_cancelled) {
            smf_set_terminate(SMF_CTX(&s_obj), 1);
        }
        int ret = smf_run_state(SMF_CTX(&s_obj));
        if (ret) {
            /* handle return code and terminate state machine */
            LOG_INF("smf return code: %d", ret);
            break;
        }
        k_msleep(1);
    }
    LOG_INF("Shutoff Cameras");
    gpio_pin_set_dt(&cam_enable, 0);
    printk("Its all over\n");
    return 0;
}
