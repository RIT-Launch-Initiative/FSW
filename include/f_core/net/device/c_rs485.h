#ifndef C_RS485_H
#define C_RS485_H

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
    CRs485(const device &uart, const gpio_dt_spec &rs485_enable, void (*uartIrqUserDataCallback)(const device *dev, void *user_data) = nullptr);

    int TransmitSynchronous(const void* data, size_t len);

    /**
     * See parent docs
     */
    int ReceiveSynchronous(void* data, size_t len);

    /**
     * See parent docs
     */
    int TransmitAsynchronous(const void* data, size_t len);

    /**
     * See parent docs
     */
    int ReceiveAsynchronous(void* data, size_t len);

    /**
     * See parent docs
     */
    int SetRxTimeout(int timeout);

private:
  static constexpr bool enableTx = true;
  static constexpr bool enableRx = false;

  const device &uart;
  const gpio_dt_spec &rs485_enable;
  const bool irqEnabled;

  /**
   * Set the RS485 transceiver to transmit or receive mode
   */
  bool setTxRx(const bool isTransmit) const {
    return gpio_pin_set_dt(&rs485_enable, isTransmit) == 0;
  }
};


#endif //C_RS485_H
