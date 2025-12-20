#ifndef C_MSGQ_MESSAGE_PORT_H
#define C_MSGQ_MESSAGE_PORT_H

#include <f_core/messaging/c_message_port.h>
#include <zephyr/kernel.h>

template <typename T>
class CLatestMessagePort : public CMessagePort<T> {
public:
    /**
     * Constructor
     */
    explicit CLatestMessagePort() {
    }

    /**
     * Destructor
     */
    ~CLatestMessagePort() override {
    }

    /**
     * See parent docs
     */
    int Send(const T& message, const k_timeout_t timeout) override {
        return 0;
    }

    /**
     * See parent docs
     */
    int Receive(T& message, const k_timeout_t timeout) override {
        return 0;
    }

    /**
     * See parent docs
     */
    void Clear() override {
    }

    /**
     * See parent docs
     */
    int AvailableSpace() override {
        return 1;
    }

private:
};

#endif // C_MSGQ_MESSAGE_PORT_H
