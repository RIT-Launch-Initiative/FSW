/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <f_core/os/c_task.h>
#include <f_core/os/n_rtos.h>
#include <zephyr/drivers/gpio.h>

#ifndef CONFIG_RADIO_MODULE_RECEIVER
#include "c_radio_module.h"
#else
#include "c_receiver_module.h"
#endif

LOG_MODULE_REGISTER(main);

static CUdpSocket udp(CIPv4{"10.2.1.1"}, 12000, 13001);

static void receiver_cb(const struct device* lora_dev, uint8_t* payload, uint16_t len, int16_t rssi, int8_t snr) {
    LOG_INF("Received %d bytes. RSSI: %d SNR: %d", len, rssi, snr);

    if (len == 3) {
        udp.SetDstPort(payload[1] << 8 | payload[0]);
        udp.TransmitAsynchronous(&payload[2], 1);
    }
}



int main() {
    CLora cLora = CLora(*DEVICE_DT_GET(DT_ALIAS(lora)));
#ifndef CONFIG_RADIO_MODULE_RECEIVER
    LOG_INF("Transmitter started");
    lora_recv_async(DEVICE_DT_GET(DT_ALIAS(lora)), &receiver_cb);
    while (true) {
        k_msleep(100);
    }
    static CRadioModule radioModule{};
#else
    LOG_INF("Receiver started");
    while (true) {
        LOG_INF("Sending");
        lora_send(DEVICE_DT_GET(DT_ALIAS(lora)), (uint8_t*)"Hello", 5);
    }


    static CReceiverModule radioModule{};
#endif
    k_msleep(2000);
    radioModule.AddTenantsToTasks();
    radioModule.AddTasksToRtos();
    radioModule.SetupCallbacks();

    NRtos::StartRtos();

#ifdef CONFIG_ARCH_POSIX
    k_sleep(K_SECONDS(300));
    NRtos::StopRtos();
#endif

    return 0;
}

