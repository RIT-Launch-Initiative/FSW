#ifndef C_LORA_H
#define C_LORA_H

#include <f_core/net/c_transciever.h>
#include <zephyr/drivers/lora.h>

class CLora : public CTransceiver {
public:
    CLora(const device* lora_dev);
    CLora(const device* lora_dev, const lora_modem_config& config);

    int TransmitSynchronous(const void* data, size_t len);

    int ReceiveSynchronous(void* data, size_t len);

    int TransmitAsynchronous(const void* data, size_t len);

    int ReceiveAsynchronous(void* data, size_t len);

    int SetRxTimeout(int timeout);

protected:
    ~CLora() = default;

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

    bool setTxRx(bool transmit);
};


#endif //C_LORA_H
