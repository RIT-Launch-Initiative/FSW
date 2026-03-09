#ifndef C_TIMED_TENANT_H
#define C_TIMED_TENANT_H

#include "f_core/os/c_tenant.h"
#include "f_core/utils/c_soft_timer.h"

class CTimedTenant : public CTenant {
public:
    /**
     * @brief Constructor
     * @param name Name of the tenant
     * @param intervalMillis Interval in milliseconds for the timeout
     */
    explicit CTimedTenant(const char* name, const uint32_t intervalMillis);

private:
    CSoftTimer timer;
};

#endif //C_TIMED_TENANT_H