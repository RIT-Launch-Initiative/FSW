#ifndef C_ZBUS_MESSAGE_PORT_H
#define C_ZBUS_MESSAGE_PORT_H

#include "zephyr/zbus/zbus.h"

#include <f_core/messaging/c_message_port.h>
#include <zephyr/kernel.h>

template <typename T>
class CZbusMessagePort : public CMessagePort<T> {
public:
    /**
     * Constructor
     * @param channel Zbus channel to use
     */
    explicit CZbusMessagePort(zbus_channel channel) : channel(channel) {

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
        return zbus_chan_pub(channel, message, timeout);
    }

    /**
     * See parent docs
     */
    int Receive(T& message, const k_timeout_t timeout) override {
        return zbus_chan_read(channel, &message, timeout);
    }

    /**
     * See parent docs
     */
    void Clear() override {
        // No clear operation for zbus channels
    }

    /**
     * See parent docs
     */
    int AvailableSpace() override {
        return 1;
    }

private:
    zbus_channel channel;
};

#endif // C_ZBUS_MESSAGE_PORT_H
