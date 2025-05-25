#ifndef C_MUTEX_H
#define C_MUTEX_H

class CMutex {
public:
    CMutex() {
        k_mutex_init(&mutexHandle);
    }

    /**
     * Lock the mutex
     * @param lockTimeout Max time to wait for the mutex to become available
     * @return Status Code. 0 on success
     */
    int Lock(k_timeout_t lockTimeout = K_NO_WAIT) {
        return k_mutex_lock(&mutexHandle, lockTimeout);
    }

    /**
     * Unlock the mutex
     * @return Status Code. 0 on success.
     */
    int Unlock(void) {
        return k_mutex_unlock(&mutexHandle);
    }

private:
    k_mutex mutexHandle;



};

#endif //C_MUTEX_H
