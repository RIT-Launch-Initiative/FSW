// Self Include
#include <f_core/net/device/c_rs485.h>

// Zephyr Includes
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(CRs485);

CRs485::CRs485(const device& uart, const gpio_dt_spec& rs485_enable) : uart(uart), rs485_enable(rs485_enable) {}

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
    LOG_ERR("TransmitAsynchronous not implemented");
    k_oops();

    return -1;
}

int CRs485::ReceiveAsynchronous(void* data, size_t len) {
    LOG_ERR("TransmitAsynchronous not implemented");
    k_oops();

    return -1;
}

int CRs485::SetTxTimeout(const int timeoutMillis) {
    txTimeoutMillis = timeoutMillis;
    return 0;
}

int CRs485::SetRxTimeout(const int timeoutMillis) {
    rxTimeoutMillis = timeoutMillis;

    return 0;
}