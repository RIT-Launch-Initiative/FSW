#ifndef C_SENSING_TENANT_H
#define C_SENSING_TENANT_H

#include <f_core/os/c_tenant.h>
#include <f_core/utils/c_soft_timer.h>

class CGnssTenant : public CTenant {
public:
    explicit CGnssTenant(const char* name)
        : CTenant(name)
    {
    }

    ~CGnssTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CSoftTimer transmitTimer{};
};

#endif //C_SENSING_TENANT_H
