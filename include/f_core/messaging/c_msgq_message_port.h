#ifndef C_MSGQ_MESSAGE_PORT_H
#define C_MSGQ_MESSAGE_PORT_H

#include <f_core/messaging/c_message_port.h>
#include <zephyr/kernel.h>


template <typename T>
class CMsgqMessagePort : public CMessagePort<T> {
public:
    CMsgqMessagePort(const uint32_t maxMessages) {
        if (k_msgq_alloc_init(&queue, sizeof(T), maxMessages)) {
            k_oops();
        }
    }

    CMsgqMessagePort(const k_msgq *queue) : queue(*queue) {};

    ~CMsgqMessagePort() override {
        k_msgq_cleanup(&queue);
    }

    int Send(const T &message, const k_timeout_t timeout) override {
        return k_msgq_put(&queue, &message, timeout);
    }

    int Receive(T message, const k_timeout_t timeout) override {
        return k_msgq_get(&queue, &message, timeout);
    }

private:
    k_msgq queue;
};



#endif //C_MSGQ_MESSAGE_PORT_H
