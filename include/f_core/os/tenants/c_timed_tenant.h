#ifndef C_TIMED_TENANT_H
#define C_TIMED_TENANT_H

#include "f_core/os/c_tenant.h"
#include "f_core/utils/c_soft_timer.h"

class CTimedTenant : public CTenant {
public:
    /**
     * @brief Constructor
     */
    explicit CTimedTenant(const char* name, uint32_t intervalMillis);

private:
    CSoftTimer timer;
};

#endif //C_TIMED_TENANT_H