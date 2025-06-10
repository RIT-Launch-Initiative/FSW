#include "f_core/radio/protocols/horus/horus.h"

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
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/types.h>
LOG_MODULE_REGISTER(radio_recv);

#define RADIO_NODE DT_ALIAS(lora0)
static const struct device *radio = DEVICE_DT_GET(RADIO_NODE);

const uint32_t carrier = 432950000;

int main() {
    struct lora_modem_config cfg{
        .frequency = carrier,
        .bandwidth = BW_250_KHZ,
        .datarate = SF_12,
        .coding_rate = CR_4_8,
        .preamble_len = 8,
        .tx_power = 20,
        .tx = false,
        .public_network = false,
    };

    int ret = lora_config(radio, &cfg);
    if (ret != 0) {
        LOG_WRN("Couldnt conf lora: %d", ret);
    }

    fs_file_t fil;
    fs_file_t_init(&fil);
    ret = fs_open(&fil, "/lfs/recv.bin", FS_O_CREATE);
    if (ret < 0) {
        LOG_ERR("Couldnt open: %d", ret);
    }

    printk("Ready\n");
    while (true) {
        char callsign[] = "KC1MOL";
        struct horus_packet_v2 data = {0};
        char buf[sizeof(callsign) + sizeof(data)] = {};

        int16_t rssi = 0;
        int8_t snr = 0;
        ret = lora_recv(radio, (uint8_t *) buf, sizeof(buf), K_FOREVER, &rssi, &snr);
        if (ret < 0) {
            LOG_WRN("Couldnt ret lora: %d", ret);
        } else {
            data = *(struct horus_packet_v2 *) (&buf[sizeof(callsign) - 1]);
            float bat_volt = (float) (data.battery_voltage) / 255 * 10;
            printk("PACKET:    %lld ms ======================\n", k_uptime_get());
            printk("Length:    %d\n", ret);
            printk("Callsign:  %.6s\n", buf);
            printk("RSSI:      %d\n", rssi);
            printk("SNR:       %d\n", snr);
            printk("ID:        %d\n", data.payload_id);
            printk("Counter:   %d\n", data.counter);
            printk("Time:      %d:%d:%d\n", data.hours, data.minutes, data.seconds);
            printk("Latitude:  %f\n", (double) data.latitude);
            printk("Longitude: %f\n", (double) data.longitude);
            printk("Altitude:  %d\n", data.altitude);
            printk("Speed:     %d\n", data.speed);
            printk("Sats:      %d\n", data.sats);
            printk("Temp:      %d\n", data.temp);
            printk("Voltage:   %.3f\n", (double) bat_volt);
            printk("\n");

            ret = fs_write(&fil, &snr, sizeof(snr));
            if (ret < 0) {
                LOG_WRN("Failed to write to file: %d", ret);
                continue;
            }

            ret = fs_write(&fil, &rssi, sizeof(rssi));
            if (ret < 0) {
                LOG_WRN("Failed to write to file: %d", ret);
                continue;
            }

            ret = fs_write(&fil, &data, sizeof(data));
            if (ret < 0) {
                LOG_WRN("Failed to write to file: %d", ret);
                continue;
            }
            fs_sync(&fil);
        }
    }
}
