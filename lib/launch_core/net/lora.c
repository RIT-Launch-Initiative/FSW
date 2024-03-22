/*
 * Copyright (c) 2023 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <launch_core/net/lora.h>

#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <memory.h>

LOG_MODULE_REGISTER(lora);

// Cached configuration. Currently under the assumption that we will only have one LoRa device in use
static struct lora_modem_config config = {
        .frequency = 915000000,
        .bandwidth = BW_125_KHZ,
        .datarate = SF_10,
        .preamble_len = 8,
        .coding_rate = CR_4_5,
        .tx_power = 4,
        .iq_inverted = false,
        .public_network = false,
        .tx = true
};

void l_lora_debug_recv_cb(const struct device *const dev, uint8_t *data, uint16_t size, int16_t rssi, int8_t snr) {
    if (size != 0) {
        LOG_INF("Received %d bytes:\n\tMem View: ", size);
        for (uint16_t i = 0; i < size; i++) LOG_INF("0x%02x ", data[i]);
        LOG_INF("\n\tVal View: %s\n", data);
        LOG_INF("\tRSSI = %ddBm\n\tSNR = %ddBm\n", rssi, snr);

        LOG_INF("\n-----------------------------------\n");
        memset(data, 0, size);
    }
}

int l_lora_tx(const struct device *const dev, uint8_t *buff, size_t len) {
    l_lora_configure(dev, true);
    int ret = lora_send(dev, buff, len);
    l_lora_configure(dev, false);
    return ret;
}

int l_lora_set_tx_rx(const struct device *const dev, bool transmit) {
    config.tx = transmit;
    return lora_config(dev, &config);
}

int l_lora_set_frequency(const struct device *const dev, uint32_t frequency) {
    config.frequency = frequency;
    return lora_config(dev, &config);
}

int l_lora_set_bandwidth(const struct device *const dev, enum lora_signal_bandwidth bandwidth) {
    config.bandwidth = bandwidth;
    return lora_config(dev, &config);
}

int l_lora_set_data_rate(const struct device *const dev, enum lora_datarate data_rate) {
    config.datarate = data_rate;
    return lora_config(dev, &config);
}

int l_lora_set_coding_rate(const struct device *const dev, enum lora_coding_rate coding_rate) {
    config.coding_rate = coding_rate;
    return lora_config(dev, &config);
}

int l_lora_set_preamble_len(const struct device *const dev, uint16_t preamble_len) {
    config.preamble_len = preamble_len;
    return lora_config(dev, &config);
}

int l_lora_set_tx_power(const struct device *const dev, int8_t tx_power) {
    config.tx_power = tx_power;
    return lora_config(dev, &config);
}