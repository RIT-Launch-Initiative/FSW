#include <string.h>
#include <f_core/net/device/c_lora.h>


CLora::CLora(const device* lora_dev) : lora_dev(lora_dev) {}

CLora::CLora(const device* lora_dev, const lora_modem_config& config) : lora_dev(lora_dev) {
    memcpy(&this->config, &config, sizeof(lora_modem_config));
}



int CLora::TransmitSynchronous(const void* data, size_t len) {
    if (setTxRx(true)) {
        return lora_send(lora_dev, (uint8_t*)data, len);
    }

    return -1;
}

int CLora::ReceiveSynchronous(void* data, size_t len) {
    // TODO Support getting RSSI and SNR
    if (setTxRx(false)) {
        return lora_recv(lora_dev, (uint8_t*)data, len, K_FOREVER, nullptr, nullptr);
    }
    return 0;
}


int CLora::TransmitAsynchronous(const void* data, size_t len) {
    if (setTxRx(true)) {
        // TODO: Have CTransciever support k_poll signals for async
        return lora_send_async(lora_dev, (uint8_t*)data, len, NULL);
    }

    return 0;
}

int CLora::ReceiveAsynchronous(void* data, size_t len) {
    // TODO Callback for lora recv async
    if (setTxRx(false)) {
        return lora_recv_async(lora_dev, nullptr);
    }
    return 0;
}

int CLora::SetRxTimeout(int timeout) {
    return 0;
}

bool CLora::setTxRx(bool transmit)  {
    config.tx = transmit;
    return lora_config(lora_dev, &config);
}



