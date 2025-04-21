/*
 * Launch Inititative
 * SPDX-License-Identifier: Apache-2.0
 */

// evil and bad includes
#include "../drivers/gnss/gnss_nmea0183.h"
#include "../drivers/gnss/gnss_nmea0183_match.h"
#include "../drivers/gnss/gnss_parse.h"

#include <string.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/modem/chat.h>
#include <zephyr/pm/device.h>
LOG_MODULE_REGISTER(gnss_ublox_m10, CONFIG_GNSS_LOG_LEVEL);

#define DT_DRV_COMPAT u_blox_m10

#define UART_RX_BUF_SZ   (256 + IS_ENABLED(CONFIG_GNSS_SATELLITES) * 512)
#define UART_TX_BUF_SZ   64
#define CHAT_RECV_BUF_SZ 256
#define CHAT_ARGV_SZ     32

uint8_t enable_pulse_on_no_lock_msg[] = {
    0xb5, 0x62,             // header
    0x06,                   // class
    0x8a,                   // id
    0xc,  0x0,              // length
    0x0,                    // version
    0x1,                    // ram
    0x0,  0x0,              // reserved2
    0x04, 0x00, 0x05, 0x40, // key: CFG-TP-LEN_TP1
    0xa0, 0x86, 0x01, 0x00, // val: 100000
    0x0d, 0xbb,             // checksum
};

struct ublox_m10_config {
    const struct device *uart;
    const struct modem_chat_script *const init_chat_script;
};

struct ublox_m10_data {
    struct gnss_nmea0183_match_data match_data;
#if CONFIG_GNSS_SATELLITES
    struct gnss_satellite satellites[CONFIG_U_BLOX_M10_SATELLITES_COUNT];
#endif

    /* UART backend */
    struct modem_pipe *uart_pipe;
    struct modem_backend_uart uart_backend;
    uint8_t uart_backend_receive_buf[UART_RX_BUF_SZ];
    uint8_t uart_backend_transmit_buf[UART_TX_BUF_SZ];

    /* Modem chat */
    struct modem_chat chat;
    uint8_t chat_receive_buf[CHAT_RECV_BUF_SZ];
    uint8_t *chat_argv[CHAT_ARGV_SZ];
};

MODEM_CHAT_MATCHES_DEFINE(unsol_matches, MODEM_CHAT_MATCH_WILDCARD("$??GGA,", ",*", gnss_nmea0183_match_gga_callback),
                          MODEM_CHAT_MATCH_WILDCARD("$??RMC,", ",*", gnss_nmea0183_match_rmc_callback),
#if CONFIG_GNSS_SATELLITES
                          MODEM_CHAT_MATCH_WILDCARD("$??GSV,", ",*", gnss_nmea0183_match_gsv_callback),
#endif
);

static int ublox_m10_resume(const struct device *dev) {
    const struct ublox_m10_config *cfg = dev->config;
    struct ublox_m10_data *data = dev->data;
    int ret;

    ret = modem_pipe_open(data->uart_pipe, K_SECONDS(10));
    if (ret < 0) {
        return ret;
    }

    ret = modem_pipe_transmit(data->uart_pipe, enable_pulse_on_no_lock_msg, sizeof(enable_pulse_on_no_lock_msg));
    if (ret < 0) {
        LOG_ERR("Couldn't write tp config: %d", ret);
        return 0;
    } else {
        LOG_INF("Wrote tp config");
    }

    ret = modem_chat_attach(&data->chat, data->uart_pipe);

    if (ret == 0) {
        ret = modem_chat_run_script(&data->chat, cfg->init_chat_script);
    }

    if (ret < 0) {
        modem_pipe_close(data->uart_pipe, K_SECONDS(10));
    }
    return ret;
}

static DEVICE_API(gnss, gnss_api) = {};

static int ublox_m10_init_nmea0183_match(const struct device *dev) {
    struct ublox_m10_data *data = dev->data;

    const struct gnss_nmea0183_match_config match_config = {
        .gnss = dev,
#if CONFIG_GNSS_SATELLITES
        .satellites = data->satellites,
        .satellites_size = ARRAY_SIZE(data->satellites),
#endif
    };

    return gnss_nmea0183_match_init(&data->match_data, &match_config);
}

static void ublox_m10_init_pipe(const struct device *dev) {
    const struct ublox_m10_config *cfg = dev->config;
    struct ublox_m10_data *data = dev->data;

    const struct modem_backend_uart_config uart_backend_config = {
        .uart = cfg->uart,
        .receive_buf = data->uart_backend_receive_buf,
        .receive_buf_size = sizeof(data->uart_backend_receive_buf),
        .transmit_buf = data->uart_backend_transmit_buf,
        .transmit_buf_size = sizeof(data->uart_backend_transmit_buf),
    };

    data->uart_pipe = modem_backend_uart_init(&data->uart_backend, &uart_backend_config);
}

static uint8_t ublox_m10_char_delimiter[] = {'\r', '\n'};

static int ublox_m10_init_chat(const struct device *dev) {
    struct ublox_m10_data *data = dev->data;

    const struct modem_chat_config chat_config = {
        .user_data = data,
        .receive_buf = data->chat_receive_buf,
        .receive_buf_size = sizeof(data->chat_receive_buf),
        .delimiter = ublox_m10_char_delimiter,
        .delimiter_size = ARRAY_SIZE(ublox_m10_char_delimiter),
        .filter = NULL,
        .filter_size = 0,
        .argv = data->chat_argv,
        .argv_size = ARRAY_SIZE(data->chat_argv),
        .unsol_matches = unsol_matches,
        .unsol_matches_size = ARRAY_SIZE(unsol_matches),
    };

    return modem_chat_init(&data->chat, &chat_config);
}

static int ublox_m10_init(const struct device *dev) {
    int ret;

    ret = ublox_m10_init_nmea0183_match(dev);
    if (ret < 0) {
        return ret;
    }

    ublox_m10_init_pipe(dev);

    struct ublox_m10_data *data = dev->data;

    ret = ublox_m10_init_chat(dev);
    if (ret < 0) {
        return ret;
    }

#if CONFIG_PM_DEVICE
    pm_device_init_suspended(dev);
#else
    ret = ublox_m10_resume(dev);
    if (ret < 0) {
        return ret;
    }
#endif

    return 0;
}

#if CONFIG_PM_DEVICE
static int ublox_m10_pm_action(const struct device *dev, enum pm_device_action action) {
    switch (action) {
        case PM_DEVICE_ACTION_RESUME:
            return ublox_m10_resume(dev);
        default:
            return -ENOTSUP;
    }
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
MODEM_CHAT_SCRIPT_EMPTY_DEFINE(ublox_m10_init_chat_script);
#endif

#define UBLOX_M10_INIT(inst)                                                                                           \
    static const struct ublox_m10_config ublox_m10_cfg_##inst = {                                                      \
        .uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                                                      \
        .init_chat_script = &ublox_m10_init_chat_script,                                                               \
    };                                                                                                                 \
                                                                                                                       \
    static struct ublox_m10_data ublox_m10_data_##inst;                                                                \
                                                                                                                       \
    PM_DEVICE_DT_INST_DEFINE(inst, ublox_m10_pm_action);                                                               \
                                                                                                                       \
    DEVICE_DT_INST_DEFINE(inst, ublox_m10_init, PM_DEVICE_DT_INST_GET(inst), &ublox_m10_data_##inst,                   \
                          &ublox_m10_cfg_##inst, POST_KERNEL, CONFIG_GNSS_INIT_PRIORITY, &gnss_api);

DT_INST_FOREACH_STATUS_OKAY(UBLOX_M10_INIT)
#undef DT_DRV_COMPAT
