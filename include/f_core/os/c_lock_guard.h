#ifndef C_LOCK_GUARD_H
#define C_LOCK_GUARD_H

class CLockGuard {
public:
    CLockGuard(CMutex &mutex) : mutex(mutex) {
        initialized = mutex.Lock();
    }

    ~CLockGuard() {
        if (initialized == 0) {
            mutex.Unlock();
        }
    }

private:
    CMutex mutex;
    int initialized = -1;
};

#endif //C_LOCK_GUARD_H
