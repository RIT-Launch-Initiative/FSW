/*
 * Copyright (c) 2023 Aaron Chan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lora_utils.h"
#include "net_utils.h"

#include <zephyr/kernel.h>
#include <string.h>

const struct device *const lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));

int init_sx1276(const struct device *const dev) {
    if (!device_is_ready(dev)) {
        printk("Device %s is not ready.\n", dev->name);
        return -ENODEV;
    }
    
    printk("Device %s is ready.\n", dev->name);
    
    return 0;
}


int lora_configure(const struct device *const dev, bool transmit) {
    struct lora_modem_config config = {
        .frequency = 915000000,
        .bandwidth = BW_125_KHZ,
        .datarate = SF_10,
        .preamble_len = 8,
        .coding_rate = CR_4_5,
        .tx_power = 4,
        .iq_inverted = false,
        .public_network = false,
        .tx = transmit
    };

    return lora_config(dev, &config);
}

void lora_debug_recv_cb(const struct device *const dev, uint8_t *data, uint16_t size, int16_t rssi, int8_t snr) {
    static RECEIVER_DATA_T receiver_data = {0}; 

	if (size != 0) {    
		printk("Received %d bytes:\n\tMem View: ",size);
		for (uint16_t i = 0; i < size; i++) printk("0x%02x ",data[i]);
        printk("\n\tVal View: %s\n", data);
		printk("\tRSSI = %ddBm\n\tSNR = %ddBm\n", rssi, snr);

        printk("\n-----------------------------------\n");
    
        uint16_t port = data[1] << 8 | data[0];
        printk("Port: %d\n", port);
        send_udp_broadcast(data + 2, size - 2, port);

        ++receiver_data.packets_sent;
        receiver_data.snr = snr;
        receiver_data.rssi = rssi;
        send_udp_broadcast((uint8_t *) &receiver_data, sizeof(receiver_data), 12001); 
        memset(data, 0, size);
	} 
}

int lora_tx(const struct device *const dev, uint8_t *buff, size_t len) {
    lora_configure(dev, true);
    int ret = lora_send(dev, buff, len);
    lora_configure(dev, false);
    return ret;
}

