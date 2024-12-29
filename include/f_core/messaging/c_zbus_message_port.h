/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef C_ZBUS_MESSAGE_PORT_H
#define C_ZBUS_MESSAGE_PORT_H

#include <zephyr/kernel.h>

template <typename T>
class CZbusMessagePort : public CMessagePort<T> {
public:
    /**
     * Constructor
     */
    explicit CZbusMessagePort(k_msgq& queue) {
    }

    /**
     * Destructor
     */
    ~CZbusMessagePort() override {
    }

    /**
     * See parent docs
     */
    int Send(const T& message, const k_timeout_t timeout) override {
        return -1;
    }

    /**
     * See parent docs
     */
    int Receive(T& message, const k_timeout_t timeout) override {
        return -1;
    }

private:
};

#endif //C_ZBUS_MESSAGE_PORT_H
