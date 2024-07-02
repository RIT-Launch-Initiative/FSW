#ifndef C_RING_BUFFER_H
#define C_RING_BUFFER_H

#include <zephyr/sys/ring_buffer.h>


template <typename T, size_t capacity>
class CRingBuffer {
public:
    CRingBuffer() {
        ring_buf_init(ringBuffer, capacitySize, &buffer);
    }

    void Get(T& data) {
        ring_buf_get(&ringBuffer, &data, sizeof(T));
    }

    void Put(const T& data) {
        ring_buf_put(&ringBuffer, &data, sizeof(T));
    }

    void Clear() {
        ring_buf_reset(&ringBuffer);
    }

    size_t GetCapacity() {
        return capacity;
    }

    bool IsEmpty() {
        return ring_buf_is_empty(&ringBuffer);
    }

private:
    const size_t capacitySize = capacity * sizeof(T);
    const uint8_t buffer[capacity * sizeof(T)] = {0};
    ring_buf ringBuffer = {0};
};

#endif //C_RING_BUFFER_H
