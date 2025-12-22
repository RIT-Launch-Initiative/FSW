#ifndef C_LATEST_MAILBOX_H
#define C_LATEST_MAILBOX_H

#include <type_traits>
#include <zephyr/sys/atomic.h>

/**
 * Overwriteable mailbox for latest value of type T.
 * A mailbox with no data published to it yet will return a zeroed value.
 *
 * @param T Type of the value to be stored in the mailbox
 */
template <typename T>
class CLatestMailbox {
    static_assert(std::is_trivially_copyable_v<T>,
                  "LatestMailbox<T> requires T to be trivially copyable");

public:
    /**
     * Constructor
     * NOTE: Latest value is memset to 0 on construction to avoid undefined state
     * @param spinsBeforeYield Number of spins to do before yielding the thread
     */
    CLatestMailbox(const uint32_t spinsBeforeYield = 64) : spinsBeforeYield(spinsBeforeYield) {
        atomic_clear(&seq);
        memset(&value, 0, sizeof(T));
    }

    ~CLatestMailbox() = default;

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
        read(out);
        return out;
    }

    /**
     * Read the latest value into a reference
     * @param out[out] Where to store the latest value
     */
    void read(T& out) const {
        atomic_val_t a;
        atomic_val_t b;
        uint32_t spins = 0;
        do {
            a = atomic_get(&seq);
            out = value;
            b = atomic_get(&seq);
            if ((a != b) || (a & 1)) {
                if (spins++ > spinsBeforeYield) {
                    spins = 0;
                    k_yield();
                }
            } else {
                break;
            }
        } while (true);
    }

private:
    // mutable since atomic_t isn't const friendly
    // Zephyr's atomic functions for seq utilize sequential consistent memory order
    mutable atomic_t seq{};
    T value{};
    const uint32_t spinsBeforeYield;
};



#endif //C_LATEST_MAILBOX_H