#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

/// stupid polling versoin
#define LILYGO_KB_BRIGHTNESS_CMD       0x01
#define LILYGO_KB_ALT_B_BRIGHTNESS_CMD 0x02

#define DT_DRV_COMPAT lilygo_tdeck_keyboard

LOG_MODULE_REGISTER(tdeck_keyboard, CONFIG_INPUT_LOG_LEVEL);

// input report key

struct tdeck_kbd_config {
    struct i2c_dt_spec bus;
};

const uint16_t key_to_input[256] = {
    ['0'] = INPUT_KEY_0, ['1'] = INPUT_KEY_1, ['2'] = INPUT_KEY_2, ['3'] = INPUT_KEY_3, ['4'] = INPUT_KEY_4,
    ['5'] = INPUT_KEY_5, ['6'] = INPUT_KEY_6, ['7'] = INPUT_KEY_7, ['8'] = INPUT_KEY_8, ['9'] = INPUT_KEY_9,

    [65] = INPUT_KEY_A,  [66] = INPUT_KEY_B,  [67] = INPUT_KEY_C,  [68] = INPUT_KEY_D,  [69] = INPUT_KEY_E,
    [70] = INPUT_KEY_F,  [71] = INPUT_KEY_G,  [72] = INPUT_KEY_H,  [73] = INPUT_KEY_I,  [74] = INPUT_KEY_J,
    [75] = INPUT_KEY_K,  [76] = INPUT_KEY_L,  [77] = INPUT_KEY_M,  [78] = INPUT_KEY_N,  [79] = INPUT_KEY_O,
    [80] = INPUT_KEY_P,  [81] = INPUT_KEY_Q,  [82] = INPUT_KEY_R,  [83] = INPUT_KEY_S,  [84] = INPUT_KEY_T,
    [85] = INPUT_KEY_U,  [86] = INPUT_KEY_V,  [87] = INPUT_KEY_W,  [88] = INPUT_KEY_X,  [89] = INPUT_KEY_Y,
    [90] = INPUT_KEY_Z,

    [97] = INPUT_KEY_A,  [98] = INPUT_KEY_B,  [99] = INPUT_KEY_C,  [100] = INPUT_KEY_D, [101] = INPUT_KEY_E,
    [102] = INPUT_KEY_F, [103] = INPUT_KEY_G, [104] = INPUT_KEY_H, [105] = INPUT_KEY_I, [106] = INPUT_KEY_J,
    [107] = INPUT_KEY_K, [108] = INPUT_KEY_L, [109] = INPUT_KEY_M, [110] = INPUT_KEY_N, [111] = INPUT_KEY_O,
    [112] = INPUT_KEY_P, [113] = INPUT_KEY_Q, [114] = INPUT_KEY_R, [115] = INPUT_KEY_S, [116] = INPUT_KEY_T,
    [117] = INPUT_KEY_U, [118] = INPUT_KEY_V, [119] = INPUT_KEY_W, [120] = INPUT_KEY_X, [121] = INPUT_KEY_Y,
    [122] = INPUT_KEY_Z,
};

struct tdeck_kbd_data {
    struct k_mutex mutex;
    uint8_t keyboard_brightness;
    uint8_t restart_brightness;
    const struct device *self;
};

static int set_brightness(const struct tdeck_kbd_config *cfg, uint8_t brightness) {
    uint8_t tx_data = LILYGO_KB_BRIGHTNESS_CMD;
    uint8_t rx_buf[2] = {0};

    int rc = -1; //i2c_write_read_dt(&cfg->bus, &tx_data, sizeof(tx_data), rx_buf, sizeof(rx_buf));
    if (rc < 0) {
        LOG_WRN("Couldn't set brightness: %d", rc);
        return rc;
    }
    return 0;
}
static int set_restart_brightness(const struct tdeck_kbd_config *cfg, uint8_t brightness) { return -1; }
static int readKey(const struct tdeck_kbd_config *cfg, uint8_t *key) {
    uint8_t tx_data[1] = {0};
    uint8_t rx_data[1] = {0};

    int rc = i2c_read_dt(&cfg->bus, rx_data, 1);

    if (rc < 0) {
        LOG_WRN("Couldnt read keyboard: %d", rc);
        return rc;
    }
    *key = rx_data[0];
    return 0;
}

static void keyboard_polling_thread(void *vp_cfg, void *vp_data, void *) {
    const struct tdeck_kbd_config *cfg = vp_cfg;
    struct tdeck_kbd_data *data = vp_data;
    const struct device *dev = data->self;
    uint8_t last_restart_brightness = 127;
    uint8_t last_keyboard_brightness = 0;

    while (true) {
        uint8_t restart_brightness = data->restart_brightness;
        uint8_t keyboard_brightness = data->keyboard_brightness;

        // default brightness
        if (last_restart_brightness != restart_brightness) {
            int res = set_restart_brightness(cfg, restart_brightness);
            if (res < 0) {
                LOG_WRN("Failed to restart brightness: %d", 0);
            }
            last_restart_brightness = restart_brightness;
        }
        // default
        if (last_keyboard_brightness != keyboard_brightness) {
            int res = set_brightness(cfg, keyboard_brightness);
            if (res < 0) {
                LOG_WRN("Failed to restart brightness: %d", 0);
            }
            last_keyboard_brightness = keyboard_brightness;
        }
        // read
        uint8_t key = 0;
        int res = readKey(cfg, &key);
        if (res < 0) {
            LOG_WRN("Failed to read key: %d", key);
        }
        if (key != 0) {
            uint16_t mapping = key_to_input[key];
            if (mapping == 0) {
                LOG_INF("Unhandled key: %d", key);
                continue;
            }
            input_report_key(data->self, mapping, 1, false, K_MSEC(5));
        }
        k_msleep(10);
    }
}
int tdeck_kbd_init(const struct device *dev) {
    // const struct tdeck_kbd_config *cfg = dev->config;
    struct tdeck_kbd_data *data = dev->data;
    k_mutex_init(&data->mutex);
    data->self = dev;
    return 0;
}

#define LILYGO_KEYBOARD_INIT(index)                                                                                    \
    static const struct tdeck_kbd_config tdeck_kbd_config_##index = {                                                  \
        .bus = I2C_DT_SPEC_INST_GET(index),                                                                            \
    };                                                                                                                 \
    static struct tdeck_kbd_data tdeck_kbd_data_##index = {                                                            \
        .keyboard_brightness = DT_INST_PROP(index, initial_brightness),                                                \
        .restart_brightness = DT_INST_PROP(index, restart_brightness),                                                 \
        .self = 0,                                                                                                     \
    };                                                                                                                 \
    DEVICE_DT_INST_DEFINE(index, tdeck_kbd_init, NULL, &tdeck_kbd_data_##index, &tdeck_kbd_config_##index,             \
                          POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);                                              \
                                                                                                                       \
    K_THREAD_DEFINE(thread_tdeck_kbd_##index, 2048, keyboard_polling_thread, &tdeck_kbd_config_##index,                \
                    &tdeck_kbd_data_##index, NULL, 0, 0, 100);

DT_INST_FOREACH_STATUS_OKAY(LILYGO_KEYBOARD_INIT)
