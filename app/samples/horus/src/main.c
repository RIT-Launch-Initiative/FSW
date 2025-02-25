#include "f_core/radio/protocols/horus/horus.h"
#include "zephyr/logging/log.h"

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void) {
    struct horus_packet_v2 packet1 = {0};
    packet1.payload_id = 5;
    packet1.altitude = 12345;
    struct horus_packet_v2 packet2 = {0};
    horus_packet_v2_encoded_buffer_t output_buf = {0};

    int i = horusv2_encode(&packet1, &output_buf);
    LOG_HEXDUMP_INF(output_buf, sizeof(output_buf), "output");
    for (int i = 0; i < 5; i++) {
        output_buf[i] = 0;
        output_buf[HORUS_ENCODED_BUFFER_SIZE - i - 1] = 0;
    }
    LOG_HEXDUMP_INF(output_buf, sizeof(output_buf), "output with modifications");

    horusv2_decode(&output_buf, &packet2);
    bool checksum_matches = horusv2_checksum_verify(&packet2);
    if (checksum_matches) {
        printk("Checksum good\n");
    } else {
        printk("Checksum bad\n");
    }
    printk("payloadID: %d, %d\n", packet1.payload_id, packet2.payload_id);
    printk("altitutde: %d, %d\n", packet1.altitude, packet2.altitude);
    return i;
}
