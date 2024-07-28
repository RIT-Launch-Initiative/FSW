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


bool sentFinished = true;
bool receiveFinished = true;

int main() {
    static constexpr std::string payload = "Launch!";

    gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
    const device* dev = DEVICE_DT_GET(DT_ALIAS(lora));
    CLora lora(*dev);

    while (true) {
        gpio_pin_toggle_dt(&led0);

        if (receiveFinished) {
        lora.ReceiveAsynchronous(
            [](const device*, uint8_t* data, uint16_t size, int16_t rssi, int8_t snr) {
                LOG_INF("Async Received: %s\tRSSI: %d\t SNR:%d\n", data, rssi, snr);
                receiveFinished = true;
            }
            );

            receiveFinished = false;
        }

        if (sentFinished) {
            lora.TransmitAsynchronous(payload.c_str(), payload.size(), nullptr);
            LOG_INF("Async Sent: %s\n", payload.c_str());
            sentFinished = false;
        }

        k_sleep(K_SECONDS(1));
    }

    return 0;
}
