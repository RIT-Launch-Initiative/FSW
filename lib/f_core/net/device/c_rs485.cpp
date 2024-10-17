// Self Include
#include <f_core/net/device/c_rs485.h>

// Zephyr Includes
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(CRs485);

CRs485::CRs485(const device& uart, const gpio_dt_spec& rs485_enable,
               void (*uartIrqUserDataCallback)(const device* dev, void* user_data)) : uart(uart),
    rs485_enable(rs485_enable), irqEnabled(uartIrqUserDataCallback != nullptr) {
    if (uartIrqUserDataCallback != nullptr) {
        uart_irq_callback_set(&uart, uartIrqUserDataCallback);
        uart_irq_rx_enable(&uart);
        uart_irq_tx_enable(&uart);
    } else {
        uart_irq_rx_disable(&uart);
        uart_irq_tx_disable(&uart);
    }
}


int CRs485::TransmitSynchronous(const void* data, size_t len) {
    if (setTxRx(enableTx) == false) {
        return -1;
    }

    size_t index = 0;
    while (index < len) {
        uart_poll_out(&uart, static_cast<const uint8_t*>(data)[index]);
        index++;
    }
    return 0;
}


int CRs485::ReceiveSynchronous(void* data, size_t len) {
    if (setTxRx(enableRx) == false) {
        return -1;
    }

    size_t index = 0;

    while (index < len) {
        if (const int ret = uart_poll_in(&uart, &static_cast<uint8_t*>(data)[index]); ret < 0) {
            return ret;
        }
        index++;
    }

    return 0;
}


int CRs485::TransmitAsynchronous(const void* data, size_t len) {
    if (unlikely(!irqEnabled)) {
        LOG_ERR("TransmitAsynchronous called without IRQ enabled");
        k_oops();
    }

    if (setTxRx(enableTx) == false) {
        return -1;
    }

    return uart_fifo_fill(&uart, static_cast<const uint8_t*>(data), static_cast<int>(len));
}

int CRs485::ReceiveAsynchronous(void* data, size_t len) {
    if (unlikely(!irqEnabled)) {
        LOG_ERR("TransmitAsynchronous called without IRQ enabled");
        k_oops();
    }

    if (setTxRx(enableRx) == false) {
        return -1;
    }

    return uart_fifo_read(&uart, static_cast<uint8_t*>(data), static_cast<int>(len));
}


int CRs485::SetRxTimeout(int timeout) {
    // TODO: Should use a k_timer to implement this in a future PR
    // Didn't see any documentation on timeouts with uart functions
    return 0;
}