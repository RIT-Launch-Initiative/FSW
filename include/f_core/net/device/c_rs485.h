#ifndef C_RS485_H
#define C_RS485_H

#include <f_core/net/c_transciever.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

class CRs485 : public CTransceiver
{
public:
    CRs485();

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

  const gpio_dt_spec &rs485_enable;
  const device &uart;

  bool setTxRx(const bool isTransmit) const
  {
    return gpio_pin_set_dt(&rs485_enable, isTransmit) == 0;
  }
};


#endif //C_RS485_H
