#ifndef C_QUEUE_H
#define C_QUEUE_H

#include <zephyr/kernel.h>

template<typename T>
class CQueue {
public:
    CQueue() {
        k_queue_init(&queue);
    }

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
        return k_queue_is_empty(&queue);
    }

private:
    k_queue queue = {0};
};

#endif //C_QUEUE_H
