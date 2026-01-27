#pragma once

// F-Core Includes
#include <f_core/net/c_transciever.h>

// Zephyr Includes
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

class CRs485 : public CTransceiver {
public:

    /**
    * Constructor
    */
    CRs485(const device &uart, const gpio_dt_spec &rs485_enable);

    int TransmitSynchronous(const void* data, size_t len) override;

    /**
     * See parent docs
     */
    int ReceiveSynchronous(void* data, size_t len) override;

    /**
     * See parent docs
     */
    int TransmitAsynchronous(const void* data, size_t len) override;

    /**
     * See parent docs
     */
    int ReceiveAsynchronous(void* data, size_t len) override;

    /**
     * See parent docs
     */
    int SetTxTimeout(int timeoutMillis) override;

 /**
     * See parent docs
     */
    int SetRxTimeout(int timeoutMillis) override;

private:
  static constexpr bool enableTx = true;
  static constexpr bool enableRx = false;

  const device &uart;
  const gpio_dt_spec &rs485_enable;

  int32_t rxTimeoutMillis = 0;
  int32_t txTimeoutMillis = 0;

  /**
   * Set the RS485 transceiver to transmit or receive mode
   */
  bool setTxRx(const bool isTransmit) const {
    return gpio_pin_set_dt(&rs485_enable, isTransmit) == 0;
  }
};


#endif //C_RS485_H
