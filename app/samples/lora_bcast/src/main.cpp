/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// std Includes
#include <string>

// F-Core Includes
#include <f_core/net/device/c_lora.h>

// Zephyr Includes
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/lora.h>

LOG_MODULE_REGISTER(main);

#ifdef CONFIG_RECEIVER
static bool receiveFinished = true;
#endif

int main() {
    CLora lora(*DEVICE_DT_GET(DT_ALIAS(lora)));

    while (true) {
#ifdef CONFIG_RECEIVER
        if (receiveFinished) {
            LOG_INF("Attempting to receive");
            lora.ReceiveAsynchronous(
                [](const device*, uint8_t* data, uint16_t size, int16_t rssi, int8_t snr) {
                    LOG_INF("Async Received: %s\tRSSI: %d\t SNR:%d", data, rssi, snr);
                    receiveFinished = true;
                }
                );

            receiveFinished = false;
        }
#else
        static constexpr std::string payload = "Launch!";
        lora.TransmitSynchronous(payload.c_str(), payload.size());
        LOG_INF("Sent: %s", payload.c_str());
#endif
        k_msleep(100);
    }

    return 0;
}
