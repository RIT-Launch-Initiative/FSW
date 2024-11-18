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
#include <f_core/net/transport/c_udp_socket.h>
#include <f_core/net/network/c_ipv4.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/lora.h>

LOG_MODULE_REGISTER(main);

#ifdef CONFIG_RECEIVER
static bool receiveFinished = true;
#endif

static CUdpSocket udp(CIPv4{"10.2.1.1"}, 12000, 12000);
int main() {
    CLora lora(*DEVICE_DT_GET(DT_ALIAS(lora)));
    std::string payload = {13001 >> 8, 13001 & 0xFF, 0b1};

    while (true) {
#ifdef CONFIG_RECEIVER
        if (receiveFinished) {
            LOG_INF("Attempting to receive");
            lora.ReceiveAsynchronous(
                [](const device*, uint8_t* data, uint16_t size, int16_t rssi, int8_t snr) {
                    LOG_INF("Async Received: %s\tRSSI: %d\t SNR:%d", data, rssi, snr);
                    receiveFinished = true;

                    if (size == 3) {
                        udp.SetDstPort(data[0] << 8 | data[1]);
                        LOG_INF("Sending %d to port: %d", data[2], data[0] << 8 | data[1]);
                        udp.TransmitSynchronous(&data[2], 1);
                    }
                }
                );

            receiveFinished = false;
        }
#else
        // std::string payload where first byte it the first half of 13001 and the second byte is the second half of 13001. The third byte is 0b1
        lora.TransmitSynchronous(payload.c_str(), payload.size());
        LOG_INF("Sent: %s", payload.c_str());

        payload[2] ^= 0b1;
#endif
        k_msleep(100);
    }

    return 0;
}
