#ifndef C_LORA_H
#define C_LORA_H

#include <zephyr/drivers/lora.h>

class CLora {
public:
    explicit CLora(const device& lora_dev);

    explicit CLora(const device& lora_dev, const lora_modem_config& config);

    int TransmitSynchronous(const void* data, size_t len);

    int ReceiveSynchronous(void* data, size_t len, int16_t *rssi, int8_t *snr, k_timeout_t timeout = K_FOREVER);

    int TransmitAsynchronous(const void* data, size_t len, k_poll_signal *signal);

    int ReceiveAsynchronous(lora_recv_cb cb);

private:
    const device* lora_dev;
    lora_modem_config config = {
        .frequency = 915000000,
        .bandwidth = BW_125_KHZ,
        .datarate = SF_12,
        .coding_rate = CR_4_5,
        .preamble_len = 8,
        .tx_power = 13,
        .tx = false,
        .iq_inverted = false,
        .public_network = false,
    };

    int setTxRx(bool transmit);
};


#endif //C_LORA_H
