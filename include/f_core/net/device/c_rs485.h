#ifndef C_RS485_H
#define C_RS485_H

#include <f_core/net/c_transciever.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/device.h>

class CRs485 : public CTransceiver
{
public:
    CRs485(const device &uart, const gpio_dt_spec &rs485_enable, void (*uartIrqUserDataCallback)(const device *dev, void *user_data) = nullptr)
        : uart(uart), rs485_enable(rs485_enable), irqEnabled(uartIrqUserDataCallback != nullptr)
    {
      if (uartIrqUserDataCallback != nullptr) {
        uart_irq_callback_set(&uart, uartIrqUserDataCallback);
          uart_irq_rx_enable(&uart);
          uart_irq_tx_enable(&uart);
      } else {
          uart_irq_rx_disable(&uart);
          uart_irq_tx_disable(&uart);

      }
    };

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

  bool setTxRx(const bool isTransmit) const
  {
    return gpio_pin_set_dt(&rs485_enable, isTransmit) == 0;
  }
};


#endif //C_RS485_H
