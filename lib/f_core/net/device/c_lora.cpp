#include <string.h>
#include <f_core/net/device/c_lora.h>


CLora::CLora(const device& lora_dev) : lora_dev(&lora_dev) {
    lora_config(this->lora_dev, &this->config);
}

CLora::CLora(const device& lora_dev, const lora_modem_config& config) : lora_dev(&lora_dev) {
    memcpy(&this->config, &config, sizeof(lora_modem_config));
    lora_config(this->lora_dev, &this->config);
}

int CLora::TransmitSynchronous(const void* data, const size_t len) {
    int ret = setTxRx(true);
    return ret == 0 ? lora_send(lora_dev, (uint8_t*) data, len) : ret;
}

int CLora::ReceiveSynchronous(void* data, const size_t len, int16_t* const rssi, int8_t* const snr, const k_timeout_t timeout) {
    int ret = setTxRx(false);
    return ret == 0 ? lora_recv(lora_dev, static_cast<uint8_t*>(data), len, timeout, rssi, snr) : ret;
}


int CLora::TransmitAsynchronous(const void* data, const size_t len, k_poll_signal* signal) {
    int ret = setTxRx(true);
    return ret == 0 ? lora_send_async(lora_dev, (uint8_t*) data, len, signal) : ret;
}

int CLora::ReceiveAsynchronous(const lora_recv_cb cb) {
    int ret = setTxRx(false);
    return ret == 0 ? lora_recv_async(lora_dev, cb) : ret;
}

int CLora::setTxRx(bool transmit) {
    return transmit != config.tx ? lora_config(lora_dev, &config) : 0;
}



