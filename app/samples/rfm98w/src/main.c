#include "zephyr/logging/log.h"

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
LOG_MODULE_REGISTER(main);

#include "rfm9Xw.h"

#include <zephyr/drivers/rtc.h>

struct __attribute__((__packed__)) HorusData {
    uint16_t payload_id;
    uint16_t seq_num;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    float latitude;
    float longitude;
    uint16_t altitude;
    uint8_t speed;
    uint8_t satellites;
    int8_t temp;
    uint8_t battery_voltage;
    uint8_t custom_data[9];
    uint16_t checksum;
};

void checksumSelf(struct HorusData *data) {
    uint16_t crc = crc16((uint8_t *) data, sizeof(struct HorusData) - sizeof(uint16_t));
    data->checksum = crc;
}

const struct device *radio_dev = DEVICE_DT_GET(DT_NODELABEL(radio));

#define LED_NODE DT_ALIAS(led)
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

const struct device *const rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

/**Adds checksum */
int32_t transmit_horus_v2(const struct device *dev, struct HorusData *data) {

    checksumSelf(data);
// LOG_INF("checksum: %04x", dat.checksum);
#define TRANSMIT_LEN 65

    uint8_t golay_encoded[TRANSMIT_LEN] = {0x0};
    int txlen = horus_l2_get_num_tx_data_bytes(sizeof(struct HorusData));
    LOG_INF("Sizeof(HorusData): %d, txlen: %d", (int) sizeof(struct HorusData), txlen);
    LOG_HEXDUMP_INF((unsigned char *) data, TRANSMIT_LEN, "dat");
    horus_l2_encode_tx_packet(golay_encoded, (unsigned char *) data, sizeof(struct HorusData));

    return transmit_4fsk_packet(dev, 8, golay_encoded, txlen);
}

static int set_date_time(const struct device *rtc) {
    int ret = 0;
    struct rtc_time tm = {
        .tm_year = 2025 - 1900,
        .tm_mon = 1,
        .tm_mday = 13,
        .tm_hour = 19,
        .tm_min = 55,
        .tm_sec = 30,
    };

    ret = rtc_set_time(rtc, &tm);
    if (ret < 0) {
        printk("Cannot write date time: %d\n", ret);
        return ret;
    }
    return ret;
}

int main(void) {

    set_date_time(rtc);

    int counter = 0;
    struct HorusData data = {
        .payload_id = 256,
        .seq_num = 1,
        .hours = 12,
        .minutes = 3,
        .seconds = 4,
        .latitude = 43.082704,
        .longitude = -77.671417,
        .altitude = 215,
        .speed = 5,
        .satellites = 4,
        .temp = 42,
        .battery_voltage = 40,
        .custom_data = {1, 2, 3, 4, 5, 6, 7, 8, 9},
    };

    while (1) {

        int ret = 0;
        struct rtc_time tm;

        ret = rtc_get_time(rtc, &tm);
        if (ret < 0) {
            printk("Cannot read date time: %d\n", ret);
            return ret;
        }
        int8_t temp = 0;
        ret = rfm9xw_read_temperature(radio_dev, &temp);
        if (ret < 0) {
            LOG_ERR("Couldnt read rfm temp: %d", ret);
        } else {
            LOG_INF("RFM Temp: %d", temp);
        }

        data.hours = tm.tm_hour;
        data.minutes = tm.tm_min;
        data.seconds = tm.tm_sec;
        data.custom_data[0] = (uint8_t) ret;
        int err = transmit_horus_v2(radio_dev, &data);
        if (err != 0) {
            LOG_ERR("RFM Returned %d", err);
        }
        LOG_INF("Tick %d", ++counter);
        k_msleep(1000);
    }
    return 0;
}
