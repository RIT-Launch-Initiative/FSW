#ifndef C_SENSING_TENANT_H
#define C_SENSING_TENANT_H

#include <f_core/os/c_tenant.h>

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
};


#endif //C_SENSING_TENANT_H
