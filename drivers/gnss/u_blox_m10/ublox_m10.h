#ifndef GNSS_UBLOX_M10_H
#define GNSS_UBLOX_M10_H
// evil and bad includes
#include "../drivers/gnss/gnss_nmea0183.h"
#include "../drivers/gnss/gnss_nmea0183_match.h"
#include "../drivers/gnss/gnss_parse.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/modem/chat.h>
#include <zephyr/pm/device.h>

#define UBLOX_M10_UART_RX_BUF_SZ   (256 + IS_ENABLED(CONFIG_GNSS_SATELLITES) * 512)
#define UBLOX_M10_UART_TX_BUF_SZ   64
#define UBLOX_M10_CHAT_RECV_BUF_SZ 256
#define UBLOX_M10_CHAT_ARGV_SZ     32

struct ublox_m10_config {
    const struct device *uart;
    const struct modem_chat_script *const init_chat_script;
    struct gpio_dt_spec reset_gpio;
    struct gpio_dt_spec timepulse_gpio;
    bool reset_on_boot;
};

struct ublox_m10_data {
    struct gpio_callback timepulse_cb_data;
    int64_t last_tick;
    k_ticks_t last_tick_delta;

    struct gnss_nmea0183_match_data match_data;
#if CONFIG_GNSS_SATELLITES
    struct gnss_satellite satellites[CONFIG_U_BLOX_M10_SATELLITES_COUNT];
#endif

    /* UART backend */
    struct modem_pipe *uart_pipe;
    struct modem_backend_uart uart_backend;
    uint8_t uart_backend_receive_buf[UBLOX_M10_UART_RX_BUF_SZ];
    uint8_t uart_backend_transmit_buf[UBLOX_M10_UART_TX_BUF_SZ];

    /* Modem chat */
    struct modem_chat chat;
    uint8_t chat_receive_buf[UBLOX_M10_CHAT_RECV_BUF_SZ];
    uint8_t *chat_argv[UBLOX_M10_CHAT_ARGV_SZ];
};
#endif