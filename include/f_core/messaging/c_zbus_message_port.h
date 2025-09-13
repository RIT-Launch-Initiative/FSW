#ifndef C_ZBUS_MESSAGE_PORT_H
#define C_ZBUS_MESSAGE_PORT_H

#include <f_core/messaging/c_message_port.h>
#include <zephyr/kernel.h>

template <typename T>
class CZbusMessagePort : public CMessagePort<T> {
public:
    /**
     * Constructor
     * @param queue Message queue to use
     */
    explicit CMsgqMessagePort() {
    }

    /**
     * Destructor
     */
    ~CMsgqMessagePort() override {
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

    /**
     * See parent docs
     */
    void Clear() override {
        return -1;
    }

    /**
     * See parent docs
     */
    int AvailableSpace() override {
        return 0;
    }

private:
};

#endif // C_ZBUS_MESSAGE_PORT_H
