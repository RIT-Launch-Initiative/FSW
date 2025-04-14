#include <stdlib.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/shell/shell.h>
#include <zephyr/types.h>
// BS ALERT
#include "sx1276/sx1276.h"

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

#define MAX_SATS 20
extern struct gnss_data last_data;
extern int current_sats;
extern int current_tracked_sats;
extern struct gnss_satellite last_sats[MAX_SATS];
extern uint64_t last_fix_uptime;

extern bool is_transmitting;
extern bool is_horus;
extern struct k_timer radio_timer;

#pragma pack(push, 1)
struct LoraPacket {
    char callsign[6];
    uint8_t current_sats;
    uint8_t sats_tracked;

    int64_t latitude;
    /** Longitudal position in nanodegrees (0 to +-180E9) */
    int64_t longitude;
    /** Bearing angle in millidegrees (0 to 360E3) */
    uint32_t bearing;
    /** Speed in millimeters per second */
    uint32_t speed;
    /** Altitude above MSL in millimeters */
    int32_t altitude;
};
#pragma pack(pop)

static struct lora_modem_config mymodem_config = {
    .frequency = 434000000,
    .bandwidth = BW_125_KHZ,
    .datarate = SF_12,
    .coding_rate = CR_4_5,
    .preamble_len = 8,
    .tx_power = 3,
    .tx = true,
    .iq_inverted = false,
    .public_network = true,
};

void init_modem() {
    const struct device *dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
    mymodem_config.tx = true;
    int ret = lora_config(dev, &mymodem_config);
    if (ret < 0) {
        printk("Bad lora cfg: %d", ret);
    }
}

static void loracfg_help(const struct shell *shell) {
    shell_print(shell, "Usage: cfg BW SF CR freq");
    shell_print(shell, "BW:\n 1: 125 khz\n 2: 250 khz\n 5: 500 khz");
    shell_print(shell, "SF:\n 6: SF6\n ...\n a: SF10\n b: SF11\n c: SF12");
    shell_print(shell, "CR:\n 5: 4/5\n 6: 4/6\n 7: 4/7\n 8: 4/8");
    shell_print(shell, "Freq: Just a number");
}
static int parse_freq(uint32_t *out, const struct shell *sh, const char *arg) {
    char *eptr;
    unsigned long val;

    val = strtoul(arg, &eptr, 0);
    if (*eptr != '\0') {
        shell_error(sh, "Invalid frequency, '%s' is not an integer", arg);
        return -EINVAL;
    }

    if (val == ULONG_MAX) {
        shell_error(sh, "Frequency %s out of range", arg);
        return -EINVAL;
    }

    *out = (uint32_t) val;
    return 0;
}

int cmd_loracfg(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 5) {
        loracfg_help(shell);
        return -1;
    }
    char bwc = argv[1][0];
    char sfc = argv[2][0];
    char crc = argv[3][0];
    const char *freqStr = argv[4];
    shell_print(shell, "bw: %c, sf: %c, cr: %c", bwc, sfc, crc);
    if (bwc == '1') {
        mymodem_config.bandwidth = BW_125_KHZ;
    } else if (bwc == '2') {
        mymodem_config.bandwidth = BW_250_KHZ;
    } else if (bwc == '5') {
        mymodem_config.bandwidth = BW_500_KHZ;
    } else {
        loracfg_help(shell);
        return 0;
    }

    if (sfc == '6') {
        mymodem_config.datarate = SF_6;
    } else if (sfc == '7') {
        mymodem_config.datarate = SF_7;
    } else if (sfc == '8') {
        mymodem_config.datarate = SF_8;
    } else if (sfc == '9') {
        mymodem_config.datarate = SF_9;
    } else if (sfc == 'a') {
        mymodem_config.datarate = SF_10;
    } else if (sfc == 'b') {
        mymodem_config.datarate = SF_11;
    } else if (sfc == 'c') {
        mymodem_config.datarate = SF_12;
    } else {
        loracfg_help(shell);
        return 0;
    }
    uint32_t freq = 0;
    int ret = parse_freq(&freq, shell, freqStr);
    if (ret != 0) {
        shell_print(shell, "Invalid Freq");
        return ret;
    }
    mymodem_config.frequency = freq;
    return 0;
}

void make_and_send_lora() {
    const struct device *dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
    mymodem_config.tx = true;
    int ret = lora_config(dev, &mymodem_config);
    if (ret < 0) {
        printk("Bad lora cfg: %d", ret);
        return;
    }

    // clang-format off
    struct LoraPacket data = {
        .callsign = {'K', 'C', '1', 'T', 'P', 'R'},
        .current_sats = (uint8_t)current_sats,
        .sats_tracked = (uint8_t)current_tracked_sats, 
        .latitude = last_data.nav_data.latitude, 
        .longitude = last_data.nav_data.longitude,
        .bearing = last_data.nav_data.bearing, 
        .speed = last_data.nav_data.bearing,
        .altitude = last_data.nav_data.altitude
    };
    // clang-format on
    ret = lora_send(dev, (uint8_t *) &data, sizeof(data));
}

const char noradio_prompt[] = "(X)uart:~$";
const char lora_prompt[] = "(L)uart:~$";

int cmd_loratx(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    if (is_transmitting) {
        shell_prompt_change(shell, noradio_prompt);
        shell_print(shell, "Stopping transmission");
        is_transmitting = false;
        k_timer_stop(&radio_timer);
    } else {
        shell_prompt_change(shell, lora_prompt);
        shell_print(shell, "Starting Lora");
        is_transmitting = true;
        is_horus = false;
        k_timer_start(&radio_timer, K_MSEC(5000), K_MSEC(5000));
    }
    return 0;
}
