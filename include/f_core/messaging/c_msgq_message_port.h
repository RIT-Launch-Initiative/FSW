#ifndef C_MSGQ_MESSAGE_PORT_H
#define C_MSGQ_MESSAGE_PORT_H

#include <f_core/messaging/c_message_port.h>
#include <zephyr/kernel.h>

template <typename T>
class CMsgqMessagePort : public CMessagePort<T> {
public:
    /**
     * Constructor
     * @param queue Message queue to use
     */
    explicit CMsgqMessagePort(k_msgq& queue) {
        this->queue = &queue;
    }

    /**
     * Destructor
     */
    ~CMsgqMessagePort() override {
        k_msgq_purge(queue);
        k_msgq_cleanup(queue);
    }

    /**
     * See parent docs
     */
    int Send(const T& message, const k_timeout_t timeout) override {
        return k_msgq_put(queue, &message, timeout);
    }

    /**
     * See parent docs
     */
    int Receive(T& message, const k_timeout_t timeout) override {
        return k_msgq_get(queue, &message, timeout);
    }

    /**
     * See parent docs
     */
    void Clear() {
        k_msgq_purge(queue);
    }

private:
    k_msgq *queue;
};

#endif // C_MSGQ_MESSAGE_PORT_H
