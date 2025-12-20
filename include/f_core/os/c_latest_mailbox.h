#ifndef C_LATEST_MAILBOX_H
#define C_LATEST_MAILBOX_H

#include <type_traits>
#include <zephyr/sys/atomic.h>

/**
 * Overwriteable mailbox for latest value of type T.
 * @param T Type of the value to be stored in the mailbox
 */
template <typename T>
class CLatestMailbox {
    static_assert(std::is_trivially_copyable_v<T>,
                  "LatestMailbox<T> requires T to be trivially copyable");

public:
    CLatestMailbox() {
        atomic_clear(&seq);
    }

    /**
     * Write a value to the mailbox
     * @param val The value to publish to the mailbox
     */
    void publish(const T& val) {
        // Odd means write in progress
        // Even means stable.
        atomic_inc(&seq);
        value = val;
        atomic_inc(&seq);
    }

    /**
     * Read the latest value stored in the mailbox
     * @return The latest value stored in the mailbox
     */
    T read() const {
        T out;
        atomic_val_t a;
        atomic_val_t b;
        do {
            a = atomic_get(&seq);
            out = value;
            b = atomic_get(&seq);
        } while ((a != b) || (a & 1));
        return out;
    }

    /**
     * Read the latest value into a reference
     * @param out[out] Where to store the latest value
     */
    void read(T& out) const {
        atomic_val_t a;
        atomic_val_t b;
        do {
            a = atomic_get(&seq);
            out = value;
            b = atomic_get(&seq);
        } while ((a != b) || (a & 1));
    }

private:
    // mutable since atomic_t isn't const friendly
    mutable atomic_t seq{};
    T value{};
};



#endif //C_LATEST_MAILBOX_H