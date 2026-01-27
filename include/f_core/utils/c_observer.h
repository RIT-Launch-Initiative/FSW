#pragma once

class CObserver {
public:
    virtual ~CObserver() = default;

    /**
     * Notify the observer of an event. The only time another thread can access a tenant, it doesn't own
     * Imagine it's an interrupt
     * @param ctx Context for tenants to interpret for themselves
     */
    virtual void Notify(void *ctx) = 0;
};

#endif //C_OBSERVER_H
