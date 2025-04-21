#include "f_core/radio/protocols/horus/horus.h"

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

extern bool do_lora;

extern void set_prompt(const struct shell *shell);
extern horus_packet_v2 get_telemetry();

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
    // const struct device *dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
    // int ret = lora_config(dev, &mymodem_config);
    // if (ret < 0) {
    // printk("Bad lora cfg: %d", ret);
    // }
}

extern void lorarx(struct fs_file_t *fil) {
    // const struct device *dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
    // mymodem_config.tx = false;
    // init_modem();
    // horus_packet_v2 packet = {0};
    // int16_t rssi;
    // int8_t snr;

    // int ret = lora_recv(dev, (unsigned char *) &packet, sizeof(packet), K_MSEC(5000), &rssi, &snr);
    // if (ret < 0 && ret != -11) {
    //     printk("LoRa recv failed: %i", ret);
    //     return;
    // }
    // horus_packet_v2 me = get_telemetry();
    // // Us
    // static char write_buf[100] = {0};
    // snprintf(write_buf, sizeof(write_buf), "%d:%d:%d, %f, %f, %d, %f, %f, %d, %d, %d\n", me.hours, me.minutes,
    //          me.seconds, me.latitude, me.longitude, me.altitude, packet.latitude, packet.longitude, packet.altitude,
    //          (int) rssi, (int) snr);
    // printk("%s", write_buf);
    // if (fil == nullptr) {
    //     return;
    // }
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
uint16_t lora_seq_number = 0;

void make_and_send_lora() {
    // const struct device *dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
    // mymodem_config.tx = true;
    // int ret = lora_config(dev, &mymodem_config);
    // if (ret < 0) {
    // printk("Bad lora cfg: %d", ret);
    // return;
    // }
    // struct horus_packet_v2 data = get_telemetry();
    //
    // clang-format on
    // ret = lora_send(dev, (uint8_t *) &data, sizeof(data));
}

int cmd_loratx(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    if (do_lora) {
        shell_print(shell, "Stopping lora");
        do_lora = false;
        set_prompt(shell);
    } else {
        shell_print(shell, "Starting Lora");
        do_lora = true;
        set_prompt(shell);
    }
    return 0;
}
