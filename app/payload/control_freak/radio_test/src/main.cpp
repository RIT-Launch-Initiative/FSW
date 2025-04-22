#include "f_core/radio/protocols/horus/horus.h"
#include "orient.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/types.h>

#define IMU_NODE DT_ALIAS(imu)
static const struct device *imu_dev = DEVICE_DT_GET(IMU_NODE);

#define PUMPEN_NODE DT_NODELABEL(pump_enable)
static const struct gpio_dt_spec pump_enable = GPIO_DT_SPEC_GET(PUMPEN_NODE, gpios);

#define RADIORST_NODE DT_ALIAS(radioreset)
static const struct gpio_dt_spec radioreset = GPIO_DT_SPEC_GET(RADIORST_NODE, gpios);

// #define GPSRST_NODE DT_ALIAS(gpsreset)
// static const struct gpio_dt_spec gpsreset = GPIO_DT_SPEC_GET(GPSRST_NODE, gpios);

// #define GPSSAFE_NODE DT_ALIAS(gpssafeboot)
// static const struct gpio_dt_spec gpstimepulse = GPIO_DT_SPEC_GET(GPSSAFE_NODE, gpios);

#define GNSS_MODEM DEVICE_DT_GET(DT_ALIAS(gnss))

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

int64_t last_timepulse_delta_cyc = 0;
int64_t last_timepulse_uptime_cyc = 0;

// static void work_handler(struct k_work *work) {
// printk("Got Pulse, %lld\n", k_cyc_to_us_near64(last_timepulse_delta_cyc));
// }
// K_WORK_DEFINE(work, work_handler);
static struct gpio_callback timepulse_cb_data;

void timepulse_ticked(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);
    int64_t cycles = k_cycle_get_64();
    int64_t ticks = k_uptime_ticks();
    last_timepulse_delta_cyc = cycles - last_timepulse_uptime_cyc;
    last_timepulse_uptime_cyc = cycles;
    // k_work_submit(&work);
}
int reset_gps() {
    // int ret;
    // if (!gpio_is_ready_dt(&gpsreset)) {
    //     printk("No GPS RST pin:(\n");
    //     return 0;
    // }
    // if (!gpio_is_ready_dt(&gpstimepulse)) {
    //     printk("No GPS safeboot pin :(\n");
    //     return 0;
    // }

    // ret = gpio_pin_configure_dt(&gpsreset, GPIO_OUTPUT_ACTIVE);
    // if (ret < 0) {
    //     printk("Failed to conf gps reset pin:(\n");
    //     return 0;
    // }

    // // Safeboot active low (send downwards before reset to enter safeboot)
    // ret = gpio_pin_configure_dt(&gpstimepulse, GPIO_OUTPUT_ACTIVE);
    // if (ret < 0) {
    //     printk("Failed to conf gps safe pin:(\n");
    //     return 0;
    // }
    // // Don't enter safeboot: pin to logic 0
    // ret = gpio_pin_set_dt(&gpstimepulse, 0);
    // if (ret < 0) {
    //     printk("couldnt set gpssafeboot pin: %d", ret);
    // }

    // k_msleep(1);

    // // Gps Reset Routine

    // ret = gpio_pin_set_dt(&gpsreset, 1);
    // if (ret < 0) {
    //     printk("couldnt set gpsreset: %d", ret);
    // }
    // k_msleep(2);
    // ret = gpio_pin_set_dt(&gpsreset, 0);
    // if (ret < 0) {
    //     printk("couldnt set gpsreset: %d", ret);
    // }
    // ret = gpio_pin_configure_dt(&gpstimepulse, GPIO_INPUT);
    // printk("GPS Reset Successfully\n");
    return 0;
}

/**
 * `0xb5, 0x62, 0x06, 0x8a, 
 * 4+[0..n], version (0), layers(0b00000FBR), reserved(0), reserved(0), keyid0, keyid1, keyid2, keyid3, val0, val1, val2, val3`
* 
* key=`0x40050004`
* val in units of 1e-6s U4  = 100000
* little endian nums
*/

// recorded from pygpsclient
// 00000000  b5 62 06 8a 0c 00 00 01  00 00 04 00 05 40 a0 86  |.b...........@..|
// 00000010  01 00 0d bb                                       |....|
// 00000014

// uint8_t enable_pulse_on_no_lock_msg[] = {
//     0xb5, 0x62,             // header
//     0x06,                   // class
//     0x8a,                   //id
//     0xc,  0x0,              // length
//     0x0,                    // version
//     0x1,                    // ram
//     0x0,  0x0,              // reserved2
//     0x04, 0x00, 0x05, 0x40, // key: CFG-TP-LEN_TP1
//     0xa0, 0x86, 0x01, 0x00, // val: 100000
//     0x0d, 0xbb,             // checksum
// };

// int gps_timepulse_cb() {
//     int ret = gpio_pin_configure_dt(&gpstimepulse, GPIO_INPUT);
//     if (ret < 0) {
//         printk("Failed to conf gps timepulse pin back to input:(\n");
//         return 0;
//     }

//     ret = gpio_pin_interrupt_configure_dt(&gpstimepulse, GPIO_INT_EDGE_TO_ACTIVE);
//     if (ret != 0) {
//         printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, gpstimepulse.port->name,
//                gpstimepulse.pin);
//         return 0;
//     }

//     gpio_init_callback(&timepulse_cb_data, timepulse_ticked, BIT(gpstimepulse.pin));
//     ret = gpio_add_callback(gpstimepulse.port, &timepulse_cb_data);
//     printk("Set up timepulse at %s pin %d: err %d\n", gpstimepulse.port->name, gpstimepulse.pin, ret);
//     return 0;
// }

// General Data
struct gnss_data last_data = {};
#define MAX_SATS 20
int current_sats = 0;
int current_tracked_sats = 0;
struct gnss_satellite last_sats[MAX_SATS] = {};
uint64_t last_fix_uptime = 0;

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data) {
    ARG_UNUSED(dev);

    last_data = *data;
    printf("GNSS DATA\n");
    if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
        last_fix_uptime = k_uptime_get();
    }
}
// GNSS_DATA_CALLBACK_DEFINE(GNSS_MODEM, gnss_data_cb);

static void gnss_satellites_cb(const struct device *dev, const struct gnss_satellite *satellites, uint16_t size) {
    ARG_UNUSED(dev);

    unsigned int tracked_count = 0;
    current_sats = size;
    for (unsigned int i = 0; i != size; ++i) {
        tracked_count += satellites[i].is_tracked;
        last_sats[i] = satellites[i];
    }
}
// GNSS_SATELLITES_CALLBACK_DEFINE(GNSS_MODEM, gnss_satellites_cb);

uint16_t horus_seq_number = 0;

horus_packet_v2 get_telemetry() {
    int8_t temp = 30;
    uint8_t volts = 0;
    float lat = (float) (((double) last_data.nav_data.latitude) / 1e9);
    float lon = (float) (((double) last_data.nav_data.longitude) / 1e9);
    uint16_t alt = last_data.nav_data.altitude / 1000;                       // mm to m
    uint8_t speed = (uint8_t) (((float) last_data.nav_data.speed) * 0.0036); // mm/sec to km/hr

    horus_seq_number++;
    horus_packet_v2 pac{
        .payload_id = CONFIG_HORUS_PAYLOAD_ID,
        .counter = horus_seq_number,
        .hours = last_data.utc.hour,
        .minutes = last_data.utc.minute,
        .seconds = (uint8_t) (last_data.utc.millisecond / 1000),
        .latitude = lat,
        .longitude = lon,
        .altitude = alt,
        .speed = speed,
        .sats = (uint8_t) last_data.info.satellites_cnt,
        .temp = temp,
        .battery_voltage = volts,
        .custom_data = {last_data.info.fix_status, last_data.info.fix_quality},
    };
    return pac;
}
bool do_lora = false;
bool do_horus = false;
bool do_lorarx = false;

const char noradio_prompt[] = "( )uart:~$";
const char lora_prompt[] = "(L)uart:~$";
const char horus_prompt[] = "(H)uart:~$";
const char both_prompt[] = "(B)uart:~$";

void set_prompt(const struct shell *shell) {
    if (do_lora && do_horus) {
        shell_prompt_change(shell, both_prompt);
    } else if (do_lora) {
        shell_prompt_change(shell, lora_prompt);
    } else if (do_horus) {
        shell_prompt_change(shell, horus_prompt);
    } else {
        shell_prompt_change(shell, noradio_prompt);
    }
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
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    // if (last_fix_uptime == 0) {
    // shell_print(shell, "Never got a fix");
    // return 0;
    // }
    uint64_t now = k_uptime_get();
    int elapsed = (now - last_fix_uptime);
    if (last_fix_uptime != 0) {
        shell_print(shell, "Got a fix %d ms ago", elapsed);
    } else {
        shell_print(shell, "Never got a gps fix");
    }
    shell_print(shell, "Cyc of pulse: %lld delta %lld us", last_timepulse_uptime_cyc,
                k_cyc_to_us_near64(last_timepulse_delta_cyc));
    // k_cycle_get_64()
    int64_t last_tp_uptime_ms = k_cyc_to_ms_near64(last_timepulse_uptime_cyc);
    shell_print(shell, "Got a pulse at %lld ms uptime (%d ms ago)", last_tp_uptime_ms, (int) (now - last_tp_uptime_ms));
    int64_t sec_len = k_cyc_to_ns_near64(last_timepulse_delta_cyc);
    shell_print(shell, "Second Length %lld ns", sec_len);

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
int cmd_lorarx(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    do_lorarx = !do_lorarx;
    shell_print(shell, "LoraRX: %d", (int) do_lorarx);
    return 0;
}

extern int cmd_servo_sweep(const struct shell *shell, size_t argc, char **argv);
extern int cmd_servo_move(const struct shell *shell, size_t argc, char **argv);
extern int cmd_servo_off(const struct shell *shell, size_t argc, char **argv);
extern int cmd_servo_on(const struct shell *shell, size_t argc, char **argv);
extern int cmd_servo_freak(const struct shell *shell, size_t argc, char **argv);
extern int cmd_servo_try_righting(const struct shell *shell, size_t argc, char **argv);

int cmd_pump_off(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    printk("Pump Off\n");
    return gpio_pin_set_dt(&pump_enable, 0);
}
int cmd_pump_on(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    printk("Pump On\n");
    return gpio_pin_set_dt(&pump_enable, 1);
}

int cmd_orient(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argv);
    ARG_UNUSED(argc);
    vec3 me = {0, 0, 0};
    int ret = find_vector(me);
    (void) ret;
    PayloadFace to_actuate = find_orientation(me);
    if (to_actuate == PayloadFace::Upright) {
        shell_print(shell, "Already upright");
    } else if (to_actuate == PayloadFace::OnItsHead || to_actuate == PayloadFace::StandingUp) {
        shell_print(shell, "We're boned");
    } else {
        shell_print(shell, "To actuate: %d", to_actuate);
        // look up servo
    }
    return 0;
}

extern void init_lora_modem();
extern int init_servo();

#define SERVO_EN DT_NODELABEL(servo_enable)
static const struct gpio_dt_spec servo_en = GPIO_DT_SPEC_GET(SERVO_EN, gpios);

#define LDO5V_EN DT_NODELABEL(ldo5v_enable)
static const struct gpio_dt_spec ldo5v_en = GPIO_DT_SPEC_GET(LDO5V_EN, gpios);

#define BUZZ_EN DT_NODELABEL(buzzer)
static const struct gpio_dt_spec buzzer = GPIO_DT_SPEC_GET(BUZZ_EN, gpios);

int cmd_buzzer_toggle(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(shell);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    gpio_pin_set_dt(&ldo5v_en, 1);
    gpio_pin_toggle_dt(&buzzer);
    return 0;
}
// clang-format off
SHELL_STATIC_SUBCMD_SET_CREATE(buzzer_subcmds, 
        SHELL_CMD(buzz, NULL, "Turn on buzzer", cmd_buzzer_toggle),
        SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(buzzer, &buzzer_subcmds, "Control Freak Control Commands", NULL);

SHELL_STATIC_SUBCMD_SET_CREATE(freak_subcmds, 
        SHELL_CMD(info, NULL, "GNSS Info to shell", cmd_gnss_info),
        SHELL_CMD(sat, NULL, "Sat Info to shell", cmd_sat_info),
        SHELL_CMD(fixstat, NULL, "Fix info", cmd_fix_info),
        SHELL_CMD(orient, NULL, "Orientation info", cmd_orient),
        SHELL_CMD(roll, NULL, "Attempt to flip: [# of attempts]", cmd_servo_try_righting),
        SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(freak, &freak_subcmds, "Control Freak Control Commands", NULL);

SHELL_STATIC_SUBCMD_SET_CREATE(radio_subcmds, 
        SHELL_CMD(rxlora, NULL, "Receive lora packets", cmd_lorarx),
        SHELL_CMD(loratx, NULL, "Enable LoRa transmission", cmd_loratx),
        SHELL_CMD(horustx, NULL, "Enable Horus transmission", cmd_horustx),
        SHELL_CMD(cfglora, NULL, "Configure lora (BW SF CR FREQ)", cmd_loracfg),
        SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(radio, &radio_subcmds, "Radio Commands", NULL);

SHELL_STATIC_SUBCMD_SET_CREATE(servo_subcmds, 
        SHELL_CMD(sweep, NULL, "sweep servo", cmd_servo_sweep),
        SHELL_CMD(move, NULL, "move servo", cmd_servo_move),
        SHELL_CMD(on, NULL, "Turn on servo power", cmd_servo_on),
        SHELL_CMD(off, NULL, "Turn off servo power", cmd_servo_off),
        SHELL_CMD(freak, NULL, "Freak servos", cmd_servo_freak),
        SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(servo, &servo_subcmds, "Servo Commands", NULL);

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

extern void lorarx(struct fs_file_t *fil);

int radio_thread(){
    while (true){
        // Maybe make packet AOT and only transmit at timeslot
        if (do_horus){
            wait_for_timeslot();
            printk("Horus\n");
            make_and_transmit_horus();
        }
        if(do_lora) {
            wait_for_timeslot();
            // lora
            printk("Lora\n");
            make_and_send_lora();
        }
        if (do_lorarx){
            lorarx(nullptr);
            k_msleep(500);
        } else if (!do_lora && !do_horus){
            k_msleep(500);
        }
    }
    return 0;
}


int main() {
    struct sensor_value sampling = {0};
     sensor_value_from_float(&sampling, 208);
    int ret = sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling);
    if (ret < 0){
        printk("Couldnt set sampling\n");
    }
    init_servo();
    // reset_gps();
    // gps_timepulse_cb();
    init_lora_modem();

    if (!gpio_is_ready_dt(&pump_enable)) {
        printk("No pump pin :(\n");
        return 0;
    }

    ret = gpio_pin_configure_dt(&pump_enable, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Failed to conf pump pin:(\n");
        return 0;
    }


    radio_thread();
    return 0;
}
