#include "zephyr/logging/log.h"

#include <f_core/radio/c_lora.h>
#include <zephyr/drivers/spi.h>

LOG_MODULE_REGISTER(CLora);

CLora::CLora(const device& lora_dev) : lora_dev(&lora_dev) {
    updateSettings();
}

CLora::CLora(const device& lora_dev, const lora_modem_config& config) : lora_dev(&lora_dev) {
    memcpy(&this->config, &config, sizeof(lora_modem_config));
    updateSettings();
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

    return lora_recv_async(lora_dev, cb, nullptr);
}

inline int CLora::setTxRx(const Direction transmitDirection) {
    const bool isTransmit = transmitDirection == TX;
    if (isTransmit == config.tx) {
        return 0;
    }

    config.tx = isTransmit;
    return lora_config(lora_dev, &config);
}

int CLora::SetBandwidth(const lora_signal_bandwidth bandwidth) {
    const lora_signal_bandwidth originalBandwidth = config.bandwidth;
    config.bandwidth = bandwidth;

    if (updateSettings() != 0) {
        config.bandwidth = originalBandwidth;
        return updateSettings();
    }

    return 0;
}

int CLora::SetSpreadingFactor(const lora_datarate spreadingFactor) {
    config.datarate = spreadingFactor;
    return updateSettings();
}

int CLora::SetCodingRate(const lora_coding_rate codingRate) {
    config.coding_rate = codingRate;
    return updateSettings();
}

int CLora::SetTxPower(int8_t txPower) {
    if (txPower < 2 || txPower > 20) {
        LOG_ERR("Tx Power %d out of range (2-20)", txPower);
        return -EINVAL;
    }

    config.tx_power = txPower;
    return updateSettings();
}

int CLora::SetFrequency(uint32_t frequency) {
    config.frequency = frequency;
    return updateSettings();
}

int CLora::SetFrequency(float frequencyMHz) {
    uint32_t frequencyHz = static_cast<uint32_t>(frequencyMHz * 1'000'000);
    return SetFrequency(frequencyHz);
}

int CLora::updateSettings() {
    const int ret = lora_config(lora_dev, &config);
    if (ret != 0) {
        LOG_ERR("Failed to update LoRa settings: %d", ret);
    }
    return ret;
}
