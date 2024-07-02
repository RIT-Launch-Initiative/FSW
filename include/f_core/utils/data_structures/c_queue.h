#ifndef C_QUEUE_H
#define C_QUEUE_H

#include <zephyr/kernel.h>

template<typename T>
class CQueue {
public:
    CQueue(const k_queue queue) : queue(queue) {}

    void Push(const T& data) {
        k_queue_append(&queue, &data);
    }

    void Pop(T& data, k_timeout_t timeout = K_NO_WAIT) {
        k_queue_get(&queue, &data, timeout);
    }

    void PeekHead(T& data, k_timeout_t timeout = K_NO_WAIT) {
        k_queue_peek(&queue, &data, timeout);
    }

    void PeekTail(T& data, k_timeout_t timeout = K_NO_WAIT) {
        k_queue_peek_tail(&queue, &data, timeout);
    }

    bool IsEmpty() {
        return k_queue_is_empty(const_cast<k_queue*>(&queue));
    }

private:
    const k_queue queue;
};

#endif //C_QUEUE_H
