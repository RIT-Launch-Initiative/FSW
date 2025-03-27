#ifndef C_SOFT_TIMER_H
#define C_SOFT_TIMER_H
#include <zephyr/kernel.h>

class CSoftTimer {
public:
    /**
    * Constructor
    * @param expirationFn Function to call when the timer expires
    * @param stopFn Function to call when the timer is stopped
    */
    explicit CSoftTimer(k_timer_expiry_t expirationFn = nullptr, k_timer_stop_t stopFn = nullptr) {
        k_timer_init(&timer, expirationFn, stopFn);
    }

    /**
    * Destructor
    */
    ~CSoftTimer() {
        StopTimer();
    }

    /**
    * Start the timer with the given expiration time
    * @param millis Time in milliseconds until the timer expires
    */
    void StartTimer(int millis) {
        // Duration (second arg) is the initial expiration time
        // Period (third arg) is the time set after each expiration
        k_timer_start(&timer, K_MSEC(millis), K_MSEC(millis));
        running = true;
    }

    /**
    * Start the timer with the given expiration time
    * @param millis Time in milliseconds until the timer expires
    * @param initialExpirationMillis Time in milliseconds to wait before the first expiration
    */
    void StartTimer(int millis, int initialExpirationMillis) {
        // Duration (second arg) is the initial expiration time
        // Period (third arg) is the time set after each expiration
        k_timer_start(&timer, K_MSEC(initialExpirationMillis), K_MSEC(millis));
        running = true;
    }

    /**
    * Stop the timer
    */
    void StopTimer() {
        k_timer_stop(&timer);
        running = false;
    }

    /**
    * Block until the timer expires
    */
    void BlockUntilExpired() {
        k_timer_status_sync(&timer);
    }

    /**
    * Get the remaining ticks until the timer expires
    * @return Remaining ticks
    */
    int64_t GetRemainingTicks() const {
        return k_timer_remaining_ticks(&timer);
    }

    /**
    * Get the remaining milliseconds until the timer expires
    * @return Remaining milliseconds
    */
    uint32_t GetRemainingMillis() {
        return k_timer_remaining_get(&timer);
    }

    /**
     * Check if the timer is running
     * @return True if the timer is running, false otherwise
     */
    bool IsRunning() const {
        return running;
    }

    /**
    * Check if the timer has expired
    * @return True if the timer has expired, false otherwise
    */
    bool IsExpired() {
        return k_timer_status_get(&timer) != 0;
    }

    /**
     * Set the user data for the timer. Ideal for passing data to callbacks
     * @param data User data to set
     */
    void SetUserData(void* data) {
        k_timer_user_data_set(&timer, data);
    }

    /**
     * Get the user data for the timer
     * @return User data
     */
    void *GetUserData() const {
        return k_timer_user_data_get(&timer);
    }

    /**
     * Get the number of times the timer expired since this function was last called
     * @return Expiration count since last read
     */
    int GetExpiredCountSinceLastRead() {
        return k_timer_status_get(&timer);
    }

private:
    k_timer timer;
    bool running = false;
};



#endif //C_SOFT_TIMER_H
