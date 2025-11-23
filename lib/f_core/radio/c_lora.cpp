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
    const lora_datarate originalSpreadingFactor = config.datarate;
    config.datarate = spreadingFactor;
    if (updateSettings() != 0) {
        config.datarate = originalSpreadingFactor;
        return updateSettings();
    }

    return updateSettings();
}

int CLora::SetCodingRate(const lora_coding_rate codingRate) {
    const lora_coding_rate originalCodingRate = config.coding_rate;
    config.coding_rate = codingRate;
    if (updateSettings() != 0) {;
        config.coding_rate = originalCodingRate;
        return updateSettings();
    }

    return 0;
}

int CLora::SetTxPower(int8_t txPower) {
    // TODO: Figure out a way to determine SX1276 vs SX1262
    // Currently expecting only SX1276 which has 2-20 dBm range
    if (txPower < 2 || txPower > 20) {
        LOG_ERR("Tx Power %d dBm is out of range (2-20 dBm)", txPower);
        return -EINVAL;
    }
    const int8_t originalTxPower = config.tx_power;
    config.tx_power = txPower;
    if (updateSettings() != 0) {
        config.tx_power = originalTxPower;
        return updateSettings();
    }

    return updateSettings();
}

int CLora::SetFrequency(uint32_t frequency) {
    if (frequency < 902'000'000 || frequency > 928'000'000) {
        LOG_ERR("Frequency %u Hz is out of range (902-928 MHz)", frequency);
        return -EINVAL;
    }

    const uint32_t originalFrequency = config.frequency;
    config.frequency = frequency;
    if (updateSettings() != 0) {
        config.frequency = originalFrequency;
        return updateSettings();
    }

    return updateSettings();
}

int CLora::SetFrequency(float frequencyMHz) {
    const uint32_t frequencyHz = static_cast<uint32_t>(frequencyMHz * 1'000'000);
    return SetFrequency(frequencyHz);
}

int CLora::updateSettings() {
    const int ret = lora_config(lora_dev, &config);
    if (ret != 0) {
        LOG_ERR("Failed to update LoRa settings: %d", ret);
    }
    return ret;
}
