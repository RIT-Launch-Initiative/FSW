/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <f_core/net/device/c_lora.h>

#include <string>


#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

int main() {
    static constexpr std::string payload = "Launch!";

    // Weird linker error with device macros + LoRa when calling this ctor
    CLora lora(*device_get_binding("lora0"));

    while (true) {
        lora.ReceiveAsynchronous(
            [](const struct device* dev, uint8_t* data, uint16_t size, int16_t rssi, int8_t snr) {
                LOG_INF("Async Received: %s\tRSSI: %d\t SNR:%d\n", data, rssi, snr);
            }
        );

        lora.TransmitAsynchronous(payload.c_str(), payload.size(), nullptr);
    }

    return 0;
}
