#include <f_core/net/device/c_lora.h>
#include <zephyr/drivers/spi.h>

CLora::CLora(const device& lora_dev) : lora_dev(&lora_dev) {
    lora_config(this->lora_dev, &this->config);
}

CLora::CLora(const device& lora_dev, const lora_modem_config& config) : lora_dev(&lora_dev) {
    memcpy(&this->config, &config, sizeof(lora_modem_config));
    lora_config(this->lora_dev, &this->config);
}

int CLora::TransmitSynchronous(const void* data, const size_t len) {
    if (const int ret = setTxRx(TX); ret != 0) {
        return ret;
    }

    return lora_send(lora_dev, static_cast<uint8_t*>(const_cast<void*>(data)), len);
}

int CLora::ReceiveSynchronous(void* data, const size_t len, int16_t* const rssi, int8_t* const snr,
                              const k_timeout_t timeout) {
    if (const int ret = setTxRx(RX); ret != 0) {
        return ret;
    }

    return lora_recv(lora_dev, static_cast<uint8_t*>(data), len, timeout, rssi, snr);
}


int CLora::TransmitAsynchronous(const void* data, const size_t len, k_poll_signal* signal) {
    if (const int ret = setTxRx(TX); ret != 0) {
        return ret;
    }
    return lora_send_async(lora_dev, static_cast<uint8_t*>(const_cast<void*>(data)), len, signal);
}

int CLora::ReceiveAsynchronous(const lora_recv_cb cb) {
    if (const int ret = setTxRx(RX); ret != 0) {
        return ret;
    }

    return lora_recv_async(lora_dev, cb);
}

inline int CLora::setTxRx(const Direction transmitDirection) {
    const bool isTransmit = transmitDirection == TX;
    if (isTransmit == config.tx) {
        return 0;
    }

    config.tx = isTransmit;
    return lora_config(lora_dev, &config);
}
