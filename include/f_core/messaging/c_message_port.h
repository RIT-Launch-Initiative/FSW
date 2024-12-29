/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CMESSAGEPORT_H
#define CMESSAGEPORT_H

#include <zephyr/kernel.h>

template <typename T>
class CMessagePort {
public:
    using MessageType = T;

    /**
     * Constructor
     */
    CMessagePort() = default;

    /**
     * Destructor
     */
    virtual ~CMessagePort() = default;

    /**
     * Send a message
     * @param message Message to send
     * @param timeout Time before sending times out
     * @return Zephyr status code
     */
    virtual int Send(const T &message, const k_timeout_t timeout = K_FOREVER) = 0;

    /**
     * Receive a message
     * @param message Message to receive
     * @param timeout Time before receiving times out
     * @return Zephyr status code
     */
    virtual int Receive(T& message, const k_timeout_t timeout = K_FOREVER) = 0;
};



#endif //CMESSAGEPORT_H
