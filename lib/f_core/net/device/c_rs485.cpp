#include <f_core/net/device/c_rs485.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CRs485);

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
    return -1;
}