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
    ~CSoftTimer() { StopTimer(); }

    /**
    * Start the timer with the given expiration time. 
    * It will run once, then stop
    * @param timeout time until the timer expires
    */
    void StartTimerOneShot(k_timeout_t timeout) {
        // Duration (second arg) is the initial expiration time
        // Period (third arg) is the time set after each expiration
        k_timer_start(&timer, timeout, K_NO_WAIT);
    }

    /**
    * Start the timer with the given expiration time. 
    * It will run repeatedly with that period
    * @param period the period of the clock tick
    */
    void StartTimerRepeating(k_timeout_t period) { k_timer_start(&timer, period, period); }

    /**
    * Stop the timer
    */
    void StopTimer() { k_timer_stop(&timer); }

    /**
    * Block until the timer expires
    */
    void BlockUntilExpired() { k_timer_status_sync(&timer); }

    /**
    * Get the remaining ticks until the timer expires
    * @return Remaining ticks
    */
    int64_t GetRemainingTicks() const { return k_timer_remaining_ticks(&timer); }

    /**
    * Get the remaining milliseconds until the timer expires
    * @return Remaining milliseconds
    */
    uint32_t GetRemainingMillis() { return k_timer_remaining_get(&timer); }

    /**
    * Check if the timer has expired
    */
    bool IsExpired() { return k_timer_status_get(&timer) != 0; }

    void SetUserData(void *user_data) { k_timer_user_data_set(&timer, user_data); }
    void *GetUserData() { return k_timer_user_data_get(&timer); }

  private:
    k_timer timer;
};

#endif //C_SOFT_TIMER_H
