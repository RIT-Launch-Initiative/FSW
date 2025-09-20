/*
 * Copyright (c) 2018 qianfan Zhao
 * Copyright (c) 2018, 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <sample_usbd.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const uint8_t hid_report_desc[] = HID_MOUSE_REPORT_DESC(2);
static enum usb_dc_status_code usb_status;

#define MOUSE_BTN_LEFT  0
#define MOUSE_BTN_RIGHT 1

enum mouse_report_idx {
    MOUSE_BTN_REPORT_IDX = 0,
    MOUSE_X_REPORT_IDX = 1,
    MOUSE_Y_REPORT_IDX = 2,
    MOUSE_WHEEL_REPORT_IDX = 3,
    MOUSE_REPORT_COUNT = 4,
};

K_MSGQ_DEFINE(mouse_msgq, MOUSE_REPORT_COUNT, 2, 1);
static K_SEM_DEFINE(ep_write_sem, 0, 1);

const struct device *imu_dev = DEVICE_DT_GET(DT_ALIAS(imu));

struct xyz {
    float x;
    float y;
    float z;
};
struct xyz calib = {0};

struct xyz measure_imu() {
    int ret = sensor_sample_fetch(imu_dev);
    if (ret != 0) {
        LOG_WRN("Failed to fetch sensor: %d", ret);
    }
    struct sensor_value xyz[3] = {0};
    ret = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, xyz);
    if (ret != 0) {
        LOG_WRN("Failed to get xyz: %d", ret);
    }
    struct xyz val = {0};
    val.x = sensor_value_to_float(&xyz[0]);
    val.y = sensor_value_to_float(&xyz[1]);
    val.z = sensor_value_to_float(&xyz[2]);
    return val;
}

int set_sampling() {
    struct sensor_value hz = {0};
    sensor_value_from_double(&hz, 1666);
    int ret = sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &hz);
    if (ret != 0) {
        LOG_ERR("Couldnt set hz: %d", ret);
        return ret;
    }
    return 0;
}

bool lmb = false;
bool rmb = false;
void imu_thread(void *, void *, void *) {
    float velx = 0;
    float vely = 0;

    while (true) {
        struct xyz tot = measure_imu();
        for (int i = 0; i < 10; i++) {
            struct xyz val = measure_imu();
            tot.x += val.x;
            tot.y += val.y;
            tot.z += val.z;
            k_msleep(1);
        }
        tot.x = tot.x / 10 - calib.x;
        tot.y = tot.y / 10 - calib.y;
        tot.z = tot.z / 10 - calib.z;
        LOG_INF("xyz: %.4f, %.4f, %.4f", (double) tot.x, (double) tot.y, (double) tot.z);

        bool report = false;
        uint8_t tmp[MOUSE_REPORT_COUNT] = {0};

        if (fabs(tot.x) > 0.2) {
            velx += -tot.x * 10;
            tmp[MOUSE_Y_REPORT_IDX] = (int) velx;
            report = true;
        } else {
            velx = 0;
        }
        if (fabs(tot.y) > 0.2) {
            vely += -tot.y * 10;
            tmp[MOUSE_X_REPORT_IDX] = (int) vely;
            report = true;
        } else {
            vely = 0;
        }

        WRITE_BIT(tmp[MOUSE_BTN_REPORT_IDX], MOUSE_BTN_LEFT, (int) lmb);
        WRITE_BIT(tmp[MOUSE_BTN_REPORT_IDX], MOUSE_BTN_RIGHT, (int) rmb);

        if (true || report) {
            if (k_msgq_put(&mouse_msgq, tmp, K_NO_WAIT) != 0) {
                LOG_ERR("Failed to put new input event");
            }
        }
    }
}

K_THREAD_DEFINE(imu, 4096, imu_thread, NULL, NULL, NULL, 4, 0, 0);

static inline void status_cb(enum usb_dc_status_code status, const uint8_t *param) { usb_status = status; }

static ALWAYS_INLINE void rwup_if_suspended(void) {
    if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
        if (usb_status == USB_DC_SUSPEND) {
            usb_wakeup_request();
            return;
        }
    }
}

static void input_cb(struct input_event *evt, void *user_data) {
    ARG_UNUSED(user_data);

    switch (evt->code) {
        case INPUT_KEY_0:
            rwup_if_suspended();
            lmb = (bool) evt->value;
            break;
        case INPUT_KEY_1:
            rwup_if_suspended();
            rmb = (bool) evt->value;
            break;
        default:
            LOG_INF("Unrecognized input code %u value %d", evt->code, evt->value);
            return;
    }
}

INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
static int enable_usb_device_next(void) {
    struct usbd_context *sample_usbd;
    int err;

    sample_usbd = sample_usbd_init_device(NULL);
    if (sample_usbd == NULL) {
        LOG_ERR("Failed to initialize USB device");
        return -ENODEV;
    }

    err = usbd_enable(sample_usbd);
    if (err) {
        LOG_ERR("Failed to enable device support");
        return err;
    }

    LOG_DBG("USB device support enabled");

    return 0;
}
#endif /* defined(CONFIG_USB_DEVICE_STACK_NEXT) */

static void int_in_ready_cb(const struct device *dev) {
    ARG_UNUSED(dev);
    k_sem_give(&ep_write_sem);
}

static const struct hid_ops ops = {
    .int_in_ready = int_in_ready_cb,
};

int main(void) {
    const struct device *hid_dev;

    if (!gpio_is_ready_dt(&led0)) {
        LOG_ERR("LED device %s is not ready", led0.port->name);
        return 0;
    }
    int ret = set_sampling();
    if (ret != 0) {
        LOG_ERR("Couldnt set hz: %d", ret);
        return ret;
    }
	k_msleep(1000);


    int steps = 100;
    for (int i = 0; i < steps; i++) {
        struct xyz meas = measure_imu();
        calib.x += meas.x / (float) steps;
        calib.y += meas.y / (float) steps;
        calib.z += meas.z / (float) steps;
        k_msleep(10);
    }
    LOG_INF("Calibration vec: %.4f, %.4f, %.4f", (double) calib.x, (double) calib.y, (double) calib.z);

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
    hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);
#else
    hid_dev = device_get_binding("HID_0");
#endif
    if (hid_dev == NULL) {
        LOG_ERR("Cannot get USB HID Device");
        return 0;
    }

    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
    if (ret < 0) {
        LOG_ERR("Failed to configure the LED pin, error: %d", ret);
        return 0;
    }

    usb_hid_register_device(hid_dev, hid_report_desc, sizeof(hid_report_desc), &ops);

    usb_hid_init(hid_dev);

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
    ret = enable_usb_device_next();
#else
    ret = usb_enable(status_cb);
#endif
    if (ret != 0) {
        LOG_ERR("Failed to enable USB");
        return 0;
    }

    while (true) {
        UDC_STATIC_BUF_DEFINE(report, MOUSE_REPORT_COUNT);

        k_msgq_get(&mouse_msgq, &report, K_FOREVER);

        ret = hid_int_ep_write(hid_dev, report, MOUSE_REPORT_COUNT, NULL);
        if (ret) {
            LOG_ERR("HID write error, %d", ret);
        } else {
            k_sem_take(&ep_write_sem, K_FOREVER);
            /* Toggle LED on sent report */
            (void) gpio_pin_toggle(led0.port, led0.pin);
        }
    }
    return 0;
}
