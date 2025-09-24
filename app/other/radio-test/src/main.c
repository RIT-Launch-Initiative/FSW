/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "types_and_other.inc"

#include <errno.h>
#include <stdalign.h>
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(radio_test);

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DEFAULT_RADIO_NODE), "No default LoRa radio specified in DT");
const struct device *lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);

#define GPS_NODE DT_ALIAS(gnss)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(GPS_NODE), "No GPS specified in DT");
const struct device *const gps_dev = DEVICE_DT_GET(GPS_NODE);

K_MUTEX_DEFINE(gps_mutex);

struct lora_modem_config config = {
    .frequency = 0,
    .bandwidth = BW_125_KHZ,
    .datarate = SF_6,
    .coding_rate = CR_4_5,
    .preamble_len = 8,
    .tx_power = 22,
    .tx = true,
    .iq_inverted = false,
    .public_network = true,
};
enum pac_size packet_size = PAC_SIZE_SMALL;

struct packet_required gps_info = {0};
bool dump_gps = false;

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data) {
    ARG_UNUSED(dev);
    int ret = k_mutex_lock(&gps_mutex, K_MSEC(20));
    if (ret != 0) {
        LOG_WRN("Failed to lock gps mutex: %d", ret);
        return;
    }
    if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
        if (dump_gps) {
            LOG_INF("Quality: %d, Status: %d, Sats: %d", data->info.fix_quality, data->info.fix_status,
                    data->info.satellites_cnt);

            LOG_INF("Lat: %lld, Long: %lld", data->nav_data.latitude, data->nav_data.longitude);
            LOG_INF("%d/%d/%d %02d:%02d:%d", data->utc.month, data->utc.month_day, data->utc.century_year,
                    data->utc.hour, data->utc.minute, data->utc.millisecond);
        }
        gps_info.time = data->utc;
        gps_info.fix = data->info.fix_status;
        gps_info.sats = data->info.satellites_cnt;
        gps_info.alt = mm_to_m(data->nav_data.altitude);
        gps_info.lat = nanodeg_to_deg(data->nav_data.latitude);
        gps_info.lon = nanodeg_to_deg(data->nav_data.longitude);
    }
    k_mutex_unlock(&gps_mutex);
}

GNSS_DATA_CALLBACK_DEFINE(gps_dev, gnss_data_cb);

int fill_packet(uint8_t *data, size_t size) {
    int ret = k_mutex_lock(&gps_mutex, K_MSEC(20));
    if (ret != 0) {
        LOG_WRN("Failed to lock gps mutex: %d", ret);
        return -1;
    }
    int len = sizeof(struct packet_required);
    memcpy(data, &gps_info, sizeof(struct packet_required));
    while (len < size) {
        len++;
        data[len] = len % 16;
    }
    k_mutex_unlock(&gps_mutex);

    return len;
}
void describe_packet(uint8_t *data, int len, int8_t snr, int16_t rssi) {
    printk("HEADER: Lat, Long, Alt, Sats, SNR, RSSI, Packet Length");
    struct packet_required pac = {0};
    printk("DATA:   %f, %f, %f, %d, %d, %d, %d", (double) pac.lat, (double) pac.lon, (double) pac.alt, (int) pac.sats,
           (int) snr, (int) rssi, (int) len);
}

int remaining_to_rx = 0;

void recv_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi, int8_t snr, void *user_data) {
    describe_packet(data, size, snr, rssi);
    remaining_to_rx--;
    if (remaining_to_rx < 0) {
        // cancel
        LOG_INF("Stopping receive. got all we expected");
        lora_recv_async(lora_dev, NULL, NULL);
    }
}

int main(void) {
    int ret;

    if (!device_is_ready(lora_dev)) {
        LOG_ERR("%s Device not ready", lora_dev->name);
        return 0;
    }

    ret = lora_config(lora_dev, &config);
    if (ret < 0) {
        LOG_ERR("Initial LoRa config failed");
        return 0;
    }
    return 0;
}

static int cmd_tx(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Transmit");
    if (argc < 1) {
        shell_error(shell, "Not enough arguments: Requires # of packets to send");
        return 0;
    }
    shell_print(shell, "Got arg0: %s", argv[0]);
    char *end = NULL;

    long num = strtol(argv[0], &end, 10);
    if (num < 1) {
        shell_print(shell, "invalid number of attempts. need at least 1");
    }
    static uint8_t packet[255] = {0};
    for (int i = 0; i < num; i++) {
        int len = fill_packet(packet, sizeof(packet));
        int ret = lora_send(lora_dev, packet, len);
        if (ret != 0) {
            LOG_WRN("Failed to send lora; %d", ret);
            continue;
        }
    }

    return 0;
}

static int cmd_rx(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "Receive");
    if (argc < 1) {
        shell_error(shell, "Not enough arguments: Requires # of packets to send");
        return 0;
    }
    shell_print(shell, "Got arg0: %s", argv[0]);
    char *end = NULL;

    long num = strtol(argv[0], &end, 10);
    if (num < 1) {
        shell_print(shell, "invalid number of attempts. need at least 1");
    }

    remaining_to_rx = num;
    int ret = lora_recv_async(lora_dev, recv_cb, NULL);
    if (ret < 0) {
        shell_error(shell, "Failed to start receiving lora; %d", ret);
        return ret;
    }

    return 0;
}

static int cmd_cancel_rx(const struct shell *shell, size_t argc, char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "Cancelling Receiving");
    lora_recv_async(lora_dev, NULL, NULL);
    remaining_to_rx = 0;
    return 0;
}
static void cmd_describe_config(struct shell *sh, int, void **) {
    shell_print(sh, "== Current Config ==");
    shell_print(sh, "\tFreq: %ud", config.frequency);
    shell_print(sh, "\tBW:   %s", bw_to_str(config.bandwidth));
    shell_print(sh, "\tSF:   SF_%d", config.datarate);
    shell_print(sh, "\tCR:   4/%d", 4 + config.coding_rate);
    shell_print(sh, "\tPLen: %s", packet_size == PAC_SIZE_SMALL ? "small" : "large");
}

static int cmd_gps_log(const struct shell *shell, size_t argc, char **argv) {
    dump_gps = !dump_gps;
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_subcmds, SHELL_CMD(tx, NULL, "Transmit: (# of attempts)", cmd_tx),
                               SHELL_CMD(tx, NULL, "Receive: (# of attempts).", cmd_rx),
                               SHELL_CMD(cfg, NULL, "Configure Paramater: (f/b/s/c/l)", cmd_describe_config),
                               SHELL_CMD(desc, NULL, "Describe Config", cmd_describe_config),
                               SHELL_CMD(gps_dump, NULL, "Toggle GPS log", cmd_gps_log),
                               SHELL_CMD(cancel_rx, NULL, "cancel receive", cmd_cancel_rx), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(test, &test_subcmds, "Radio Test Commands", NULL);
