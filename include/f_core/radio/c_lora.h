#ifndef C_LORA_H
#define C_LORA_H

#include <zephyr/drivers/lora.h>

class CLora {
public:
    /**
     * Constructor
     * @param lora_dev[in] The LoRa device to use
     */
    explicit CLora(const device& lora_dev);

    /**
     * Constructor
     * @param lora_dev[in] The LoRa device to use
     * @param config[in] Configuration for the LoRa modem
     */
    explicit CLora(const device& lora_dev, const lora_modem_config& config);

    /**
     * Synchronously transmit data (blocking)
     * @param data[in] Data to transmit
     * @param len[in] Size of data
     * @return Zephyr status code
     */
    int TransmitSynchronous(const void* data, size_t len);

    /**
     * Receive data synchronously (blocking)
     * @param data[out] Buffer to receive data into
     * @param len[out] Size of buffer for receiving data
     * @param rssi[in] Pointer to received signal strength indicator from received packet
     * @param snr[in] Pointer to signal to noise ratio from received packet
     * @param timeout[out] Timeout for receiving data
     * @return Zephyr status code
     */
    int ReceiveSynchronous(void* data, size_t len, int16_t *rssi, int8_t *snr, k_timeout_t timeout = K_FOREVER);

    /**
     * Transmit data asynchronously (non-blocking)
     * @param data[in] Data to transmit
     * @param len[in] Size of data
     * @param signal[in] Signal to notify when transmission is complete
     * @return Zephyr status code
     */
    int TransmitAsynchronous(const void* data, size_t len, k_poll_signal *signal);

    /**
     * Receive data asynchronously (non-blocking)
     * @param cb Callback to run when a packet is received
     * @return Zephyr status code
     */
    int ReceiveAsynchronous(lora_recv_cb cb);

    int SetBandwidth(const lora_signal_bandwidth bandwidth);

    int SetSpreadingFactor(const lora_datarate spreadingFactor);

    int SetCodingRate(const lora_coding_rate codingRate);

    int SetTxPower(int8_t txPower);

    int SetFrequency(uint32_t frequency);

    int SetFrequency(float frequencyMHz);

private:
    const device* lora_dev;
    lora_modem_config config = {
        .frequency = 906900000,
        .bandwidth = BW_125_KHZ,
        .datarate = SF_12,
        .coding_rate = CR_4_8,
        .preamble_len = 8,
        .tx_power = 20,
        .tx = false,
        .iq_inverted = false,
        .public_network = false,
    };

    enum Direction {
      RX = 0,
      TX
    };

   /**
    * Set the LoRa modem to transmit or receive mode
    * @param transmitDirection true to transmit, false to receive
    * @return Zephyr status code
    */
    int setTxRx(Direction transmitDirection);

    int updateSettings();
};


#endif //C_LORA_H
