#ifndef C_QUEUE_H
#define C_QUEUE_H
#include <zephyr/kernel.h>

class CQueue {
public:

    void push(const T& data) {
        k_queue_append(&queue, &data);
    }

    void pop(T& data) {
        k_queue_get(&queue, &data, K_FOREVER);
    }



private:
    const k_queue queue;
};

#endif //C_QUEUE_H
