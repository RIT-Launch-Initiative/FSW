#include "f_core/radio/protocols/horus/horus.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/types.h>

#define PUMPEN_NODE DT_NODELABEL(pump_enable)
static const struct gpio_dt_spec pump_enable = GPIO_DT_SPEC_GET(PUMPEN_NODE, gpios);

#define RADIORST_NODE DT_ALIAS(radioreset)
static const struct gpio_dt_spec radioreset = GPIO_DT_SPEC_GET(RADIORST_NODE, gpios);

#define GPSRST_NODE DT_ALIAS(gpsreset)
static const struct gpio_dt_spec gpsreset = GPIO_DT_SPEC_GET(GPSRST_NODE, gpios);

#define GPSSAFE_NODE DT_ALIAS(gpssafeboot)
static const struct gpio_dt_spec gpssafeboot = GPIO_DT_SPEC_GET(GPSSAFE_NODE, gpios);

#define GNSS_MODEM DEVICE_DT_GET(DT_ALIAS(gnss))

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

int reset_gps() {
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

    ret = gpio_pin_configure_dt(&gpssafeboot, GPIO_INPUT);
    if (ret < 0) {
        printk("Failed to conf gps timepulse pin back to input:(\n");
        return 0;
    }

    printk("GPS Reset Successfully\n");
    return 0;
}

// General Data
struct gnss_data last_data = {};
#define MAX_SATS 20
int current_sats = 0;
int current_tracked_sats = 0;
struct gnss_satellite last_sats[MAX_SATS] = {};
uint64_t last_fix_uptime = 0;

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data) {
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

bool is_transmitting = false;
bool is_horus = true;
K_TIMER_DEFINE(radio_timer, NULL, NULL);

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
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

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
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
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
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

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

// Radio transmission
extern void make_and_transmit_horus();
extern void make_and_send_lora();

// Radio Commands
extern int cmd_horustx(const struct shell *shell, size_t argc, char **argv);
extern int cmd_loracfg(const struct shell *shell, size_t argc, char **argv);
extern int cmd_loratx(const struct shell *shell, size_t argc, char **argv);

extern int cmd_servo(const struct shell *shell, size_t argc, char **argv);
extern int cmd_servo_off(const struct shell *shell, size_t argc, char **argv);
extern int cmd_servo_on(const struct shell *shell, size_t argc, char **argv);

int cmd_pump_off(const struct shell *shell, size_t argc, char **argv) {
    printk("Pump Off\n");
    return gpio_pin_set_dt(&pump_enable, 0);
}
int cmd_pump_on(const struct shell *shell, size_t argc, char **argv) {
    printk("Pump On\n");
    return gpio_pin_set_dt(&pump_enable, 1);
}

extern void init_modem();
extern int init_servo();

// clang-format off
SHELL_STATIC_SUBCMD_SET_CREATE(freak_subcmds, 
        SHELL_CMD(info, NULL, "GNSS Info to shell", cmd_gnss_info),
        SHELL_CMD(sat, NULL, "Sat Info to shell", cmd_sat_info),
        SHELL_CMD(fixstat, NULL, "Fix info", cmd_fix_info),
        SHELL_CMD(loratx, NULL, "Transmit lora packet (according to cfglora settings)", cmd_loratx),
        SHELL_CMD(horustx, NULL, "Transmit horus packet (according to cfghorus settings)", cmd_horustx),
        SHELL_CMD(cfglora, NULL, "Configure lora (BW SF CR)", cmd_loracfg),
        SHELL_CMD(servo, NULL, "Send Servo Command", cmd_servo),
        SHELL_CMD(servoon, NULL, "Turn on servo power", cmd_servo_on),
        SHELL_CMD(servooff, NULL, "Turn off servo power", cmd_servo_off),
        SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(freak, &freak_subcmds, "Control Freak Control Commands", NULL);

SHELL_STATIC_SUBCMD_SET_CREATE(pump_subcmds, 
        SHELL_CMD(on, NULL, "Pump on", cmd_pump_on),
        SHELL_CMD(off, NULL, "Pump off", cmd_pump_off),
        SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(pump, &pump_subcmds, "Pump Commands", NULL);


void wait_for_timeslot(){
    // Next avail timeslot start
    // now
    // k_sleep(next time - now);
    k_msleep(5000);
}

int radio_thread(){
    while (true){
        // Maybe make packet AOT and only transmit at timeslot
        wait_for_timeslot();
        if (is_transmitting){
        // wait till timesync
        if (is_horus == true){
            // horus
            printk("Horus\n");
            make_and_transmit_horus();
        } else {
            // lora
            printk("Lora\n");
            make_and_send_lora();
            }
        }
    }
    return 0;
}


int main() {
    init_servo();
    reset_gps();
    init_modem();

    if (!gpio_is_ready_dt(&pump_enable)) {
        printk("No pump pin :(\n");
        return 0;
    }

    int ret = gpio_pin_configure_dt(&pump_enable, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Failed to conf pump pin:(\n");
        return 0;
    }


    radio_thread();
    return 0;
}
