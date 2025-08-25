#ifndef C_TIMED_TENANT_H
#define C_TIMED_TENANT_H

#include "f_core/os/c_tenant.h"
#include "f_core/utils/c_soft_timer.h"

class CTimedTenant : public CTenant {
public:
    /**
     * @brief Constructor
     */
    explicit CTimedTenant(const char* name)
        : CTenant(name) {}

    /**
     * @brief See parent docs.
     */
    void Startup() override;

    /**
     * @brief See parent docs.
     */
    void Run() override {
        // Should be no behavior
    }

private:
    CSoftTimer timer;

};

#endif //C_TIMED_TENANT_H