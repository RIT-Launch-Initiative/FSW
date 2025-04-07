
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/types.h>

// evil but i want it
// #include "../../zephyr/drivers/gnss/gnss_dump.h"

#define GPSRST_NODE DT_ALIAS(gpsreset)
static const struct gpio_dt_spec gpsreset = GPIO_DT_SPEC_GET(GPSRST_NODE, gpios);

#define GPSSAFE_NODE DT_ALIAS(gpssafeboot)
static const struct gpio_dt_spec gpssafeboot = GPIO_DT_SPEC_GET(GPSSAFE_NODE, gpios);

#define GNSS_MODEM DEVICE_DT_GET(DT_ALIAS(gnss))

int resetGPS() {
    int ret;
    if (!gpio_is_ready_dt(&gpsreset)) {
        printk("No GPS RST pin:(\n");
        return 0;
    }
    if (!gpio_is_ready_dt(&gpssafeboot)) {
        printk("No GPS safeboot pin :(\n");
        return 0;
    }

    ret = gpio_pin_configure_dt(&gpsreset, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to conf gps reset pin:(\n");
        return 0;
    }

    // Safeboot active low (send downwards before reset to enter safeboot)
    ret = gpio_pin_configure_dt(&gpssafeboot, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Failed to conf gps safe pin:(\n");
        return 0;
    }
    // Don't enter safeboot: pin to logic 0
    ret = gpio_pin_set_dt(&gpssafeboot, 0);
    if (ret < 0) {
        printk("couldnt set gpssafeboot pin: %d", ret);
    }

    k_msleep(1);

    // Gps Reset Routine

    ret = gpio_pin_set_dt(&gpsreset, 1);
    if (ret < 0) {
        printk("couldnt set gpsreset: %d", ret);
    }
    k_msleep(2);
    ret = gpio_pin_set_dt(&gpsreset, 0);
    if (ret < 0) {
        printk("couldnt set gpsreset: %d", ret);
    }
    printk("GPS Reset Successfully\n");
    return 0;
}

static struct gnss_data last_data = {};
#define MAX_SATS 20
int current_sats = 0;
int current_tracked_sats = 0;
static struct gnss_satellite last_sats[MAX_SATS] = {};
static uint64_t last_fix_uptime = 0;

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data) {
    uint64_t timepulse_ns;
    k_ticks_t timepulse;

    last_data = *data;

    if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
        last_fix_uptime = k_uptime_get();
    }
}
GNSS_DATA_CALLBACK_DEFINE(GNSS_MODEM, gnss_data_cb);

static void gnss_satellites_cb(const struct device *dev, const struct gnss_satellite *satellites, uint16_t size) {
    unsigned int tracked_count = 0;
    current_sats = size;
    for (unsigned int i = 0; i != size; ++i) {
        tracked_count += satellites[i].is_tracked;
        last_sats[i] = satellites[0];
    }
}
GNSS_SATELLITES_CALLBACK_DEFINE(GNSS_MODEM, gnss_satellites_cb);

int main() {
    resetGPS();

    return 0;
}
#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

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
    .datarate = SF_10,
    .coding_rate = CR_4_5,
    .preamble_len = 8,
    .tx_power = 20,
};

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

static int cmd_loracfg(const struct shell *shell, size_t argc, char **argv) {
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

static int cmd_loratx(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Sending LoRa");
    const struct device *dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
    mymodem_config.tx = true;
    int ret = lora_config(dev, &mymodem_config);
    if (ret < 0) {
        shell_print(shell, "Error configuring lora: %d", ret);
        return ret;
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
    shell_print(shell, "Returned: %d (%d)", ret, sizeof(data));
    return 0;
}

static const char *gnss_system_to_str(enum gnss_system system) {
    switch (system) {
        case GNSS_SYSTEM_GPS:
            return "GPS";
        case GNSS_SYSTEM_GLONASS:
            return "GLONASS";
        case GNSS_SYSTEM_GALILEO:
            return "GALILEO";
        case GNSS_SYSTEM_BEIDOU:
            return "BEIDOU";
        case GNSS_SYSTEM_QZSS:
            return "QZSS";
        case GNSS_SYSTEM_IRNSS:
            return "IRNSS";
        case GNSS_SYSTEM_SBAS:
            return "SBAS";
        case GNSS_SYSTEM_IMES:
            return "IMES";
    }

    return "unknown";
}
static int cmd_fix_info(const struct shell *shell, size_t argc, char **argv) {
    if (last_fix_uptime == 0) {
        shell_print(shell, "Never got a fix");
        return 0;
    }
    uint64_t now = k_uptime_get();
    int elapsed = (now - last_fix_uptime);
    shell_print(shell, "Got a fix %d ms ago", elapsed);
    return 0;
}

static const char *gnss_fix_status_to_str(enum gnss_fix_status fix_status) {
    switch (fix_status) {
        case GNSS_FIX_STATUS_NO_FIX:
            return "NO_FIX";
        case GNSS_FIX_STATUS_GNSS_FIX:
            return "GNSS_FIX";
        case GNSS_FIX_STATUS_DGNSS_FIX:
            return "DGNSS_FIX";
        case GNSS_FIX_STATUS_ESTIMATED_FIX:
            return "ESTIMATED_FIX";
    }

    return "unknown";
}

static const char *gnss_fix_quality_to_str(enum gnss_fix_quality fix_quality) {
    switch (fix_quality) {
        case GNSS_FIX_QUALITY_INVALID:
            return "INVALID";
        case GNSS_FIX_QUALITY_GNSS_SPS:
            return "GNSS_SPS";
        case GNSS_FIX_QUALITY_DGNSS:
            return "DGNSS";
        case GNSS_FIX_QUALITY_GNSS_PPS:
            return "GNSS_PPS";
        case GNSS_FIX_QUALITY_RTK:
            return "RTK";
        case GNSS_FIX_QUALITY_FLOAT_RTK:
            return "FLOAT_RTK";
        case GNSS_FIX_QUALITY_ESTIMATED:
            return "ESTIMATED";
    }

    return "unknown";
}

int gnss_dump_nav_data(char *str, uint16_t strsize, const struct navigation_data *nav_data) {
    int ret;
    const char *fmt = "nav_data:\n  lat: %s%lli.%09lli,\n  long : %s%lli.%09lli\n  "
                      "bearing %u.%03u\n  speed %u.%03u\n  altitude: %s%i.%03i";
    const char *lat_sign = nav_data->latitude < 0 ? "-" : "";
    const char *lon_sign = nav_data->longitude < 0 ? "-" : "";
    const char *alt_sign = nav_data->altitude < 0 ? "-" : "";

    ret = snprintk(str, strsize, fmt, lat_sign, llabs(nav_data->latitude) / 1000000000,
                   llabs(nav_data->latitude) % 1000000000, lon_sign, llabs(nav_data->longitude) / 1000000000,
                   llabs(nav_data->longitude) % 1000000000, nav_data->bearing / 1000, nav_data->bearing % 1000,
                   nav_data->speed / 1000, nav_data->speed % 1000, alt_sign, abs(nav_data->altitude) / 1000,
                   abs(nav_data->altitude) % 1000);

    return (strsize < ret) ? -ENOMEM : 0;
}

int gnss_dump_time(char *str, uint16_t strsize, const struct gnss_time *utc) {
    int ret;
    const char *fmt = "time:\n  hour: %u\n  minute: %u\n  millisecond %u\n  month_day %u, "
                      "month: %u\n  year: %u";

    ret = snprintk(str, strsize, fmt, utc->hour, utc->minute, utc->millisecond, utc->month_day, utc->month,
                   utc->century_year);

    return (strsize < ret) ? -ENOMEM : 0;
}

int gnss_dump_info(char *str, uint16_t strsize, const struct gnss_info *info) {
    int ret;
    const char *fmt = "gnss_info:\n  satellites_cnt: %u\n  hdop: %u.%u\n  fix_status: %s, "
                      "\n  fix_quality: %s";

    ret = snprintk(str, strsize, fmt, info->satellites_cnt, info->hdop / 1000, info->hdop % 1000,
                   gnss_fix_status_to_str(info->fix_status), gnss_fix_quality_to_str(info->fix_quality));

    return (strsize < ret) ? -ENOMEM : 0;
}

static int cmd_gnss_info(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "gnss info:");
    static char dumpstr[256] = {0};
    const struct gnss_info *info = &last_data.info;
    const struct gnss_time *time = &last_data.utc;
    const struct navigation_data *nav_data = &last_data.nav_data;
    gnss_dump_nav_data(dumpstr, sizeof(dumpstr), nav_data);
    shell_print(shell, "%s", dumpstr);

    gnss_dump_time(dumpstr, sizeof(dumpstr), time);
    shell_print(shell, "%s", dumpstr);

    gnss_dump_info(dumpstr, sizeof(dumpstr), info);
    shell_print(shell, "%s", dumpstr);
    return 0;
}

static int cmd_sat_info(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "(tracked)(prn, snr) (elavation, azimuth) - system");
    if (current_sats == 0) {
        shell_print(shell, "No satelites");
    }
    for (int i = 0; i < current_sats; i++) {
        struct gnss_satellite *sat = &last_sats[i];
        shell_print(shell, "Sat(%d): (%d,%d), (%d %d) - %s", (int) sat->is_tracked, sat->prn, sat->snr, sat->elevation,
                    sat->azimuth, gnss_system_to_str(sat->system));
    }
    return 0;
}
// clang-format off
SHELL_STATIC_SUBCMD_SET_CREATE(grim_subcmds, 
        SHELL_CMD(info, NULL, "GNSS Info to shell", cmd_gnss_info),
        SHELL_CMD(sat, NULL, "Sat Info to shell", cmd_sat_info),
        SHELL_CMD(fixstat, NULL, "Fix info", cmd_fix_info),
        SHELL_CMD(loratx, NULL, "Transmit lora packet (according to lora shell settings)", cmd_loratx),
        SHELL_CMD(cfglora, NULL, "Configure lora (BW SF CR)", cmd_loracfg),
        SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(freak, &grim_subcmds, "Grim Control Commands", NULL);
