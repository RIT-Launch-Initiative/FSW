#ifndef C_SOFT_TIMER_H
#define C_SOFT_TIMER_H

class CSoftTimer {
public:
    CSoftTimer(k_timer_expiry_t expirationFn = nullptr, k_timer_stop_t stopFn = nullptr) {
        k_timer_init(&timer, expirationFn, stopFn);
    }

    ~CSoftTimer() {
        StopTimer();
    }

    void StartTimer(int millis) {
        // Duration (second arg) is the initial expiration time
        // Period (third arg) is the time set after each expiration
        k_timer_start(&timer, K_MSEC(millis), K_MSEC(millis));
    }

    void StopTimer() {
        k_timer_stop(&timer);
    }

    void BlockUntilExpired() {
        k_timer_status_sync(&timer);
    }

    int64_t GetRemainingTicks() const {
        return k_timer_remaining_ticks(&timer);
    }

    uint32_t GetRemainingMillis() {
        return k_timer_remaining_get(&timer);
    }

    bool IsExpired() {
        return k_timer_status_get(&timer) == 0;
    }

private:
    k_timer timer;
};



#endif //C_SOFT_TIMER_H
