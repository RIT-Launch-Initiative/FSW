#ifndef C_MSGQ_MESSAGE_PORT_H
#define C_MSGQ_MESSAGE_PORT_H

#include <f_core/messaging/c_message_port.h>
#include <zephyr/kernel.h>

template <typename T>
class CMsgqMessagePort : public CMessagePort<T> {
public:
    explicit CMsgqMessagePort(const uint32_t maxMessages) {
        if (k_msgq_alloc_init(this->queue, sizeof(T), maxMessages) != 0) {
            k_oops();
        }
    }

    explicit CMsgqMessagePort(k_msgq& queue) {
        this->queue = &queue;
    }

    ~CMsgqMessagePort() override {
        k_msgq_cleanup(queue);
    }

    int Send(const T& message, const k_timeout_t timeout) override {
        return k_msgq_put(queue, &message, timeout);
    }

    int Receive(T& message, const k_timeout_t timeout) override {
        return k_msgq_get(queue, &message, timeout);
    }

private:
    k_msgq *queue;
};

#endif // C_MSGQ_MESSAGE_PORT_H
