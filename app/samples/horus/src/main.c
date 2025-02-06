#include "zephyr/logging/log.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include "f_core/protocols/horus/horus.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void) {
	struct horus_packet_v2 asdf = {0};
	asdf.payload_id = 5;
	asdf.altitude = 12345;
	struct horus_packet_v2 asdf2 = {0};
	horus_packet_v2_encoded_buffer_t output_buf = {0};

	int i  = horusv2_encode(&asdf, &output_buf);
	LOG_HEXDUMP_INF(output_buf, sizeof(output_buf), "output");
	for (int i = 0; i < 10; i++){
		output_buf[i] = 0;
		output_buf[HORUS_ENCODED_BUFFER_SIZE - i - 1] = 0;
	}
	LOG_HEXDUMP_INF(output_buf, sizeof(output_buf), "output with modifications");

	int res = horusv2_decode(&output_buf, &asdf2);
	bool checksum_matches = horusv2_checksum_verify(&asdf2);
	if (checksum_matches){
		printk("Checksum good\n");
	} else {
		printk("Checksum bad\n");
	}
	printk("Res: %d\n", res);
	printk("payloadID: %d, %d\n", asdf.payload_id, asdf2.payload_id);
	printk("altitutde: %d, %d\n", asdf.altitude, asdf2.altitude);
	return i;
}
