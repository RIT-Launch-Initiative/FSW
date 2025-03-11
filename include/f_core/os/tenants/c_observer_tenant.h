#ifndef C_OBSERVER_H
#define C_OBSERVER_H

#include "f_core/os/c_tenant.h"

class CObserverTenant : public CTenant { // CObserverTenant inheriting from CTenant avoids ambi
public:
    CObserverTenant(const char* name) : CTenant(name) {}

    /**
     * Notify the observer of an event. The only time another thread can access a tenant, it doesn't own
     * Imagine it's an interrupt
     * @param ctx Context for tenants to interpret for themselves
     */
    virtual void Notify(void *ctx) = 0;
};

#endif //C_OBSERVER_H
