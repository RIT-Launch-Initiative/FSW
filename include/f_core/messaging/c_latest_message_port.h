#pragma once

#include "f_core/messaging/c_message_port.h"
#include "f_core/os/c_latest_mailbox.h"

#include <zephyr/kernel.h>

template <typename T>
class CLatestMessagePort : public CMessagePort<T> {
public:
    /**
     * Constructor
     */
    explicit CLatestMessagePort(const uint32_t spinsBeforeYield = 64)
        : mailbox(spinsBeforeYield) {}

    /**
     * Destructor
     */
    ~CLatestMessagePort() override = default;

    /**
     * See parent docs
     */
    int Send(const T& message, const k_timeout_t timeout) override {
        mailbox.publish(message);
        return 0;
    }

    /**
     * See parent docs
     */
    int Receive(T& message, const k_timeout_t timeout) override {
        mailbox.read(message);
        return 0;
    }

    /**
     * See parent docs
     */
    void Clear() override {
        // noop
    }

    /**
     * See parent docs
     */
    size_t AvailableSpace() override {
        return 1;
    }

private:
    CLatestMailbox<T> mailbox;
};


