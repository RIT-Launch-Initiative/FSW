#ifndef C_SENSING_TENANT_H
#define C_SENSING_TENANT_H

#include <f_core/os/c_tenant.h>

class CSensingTenant : public CTenant {
public:
    explicit CSensingTenant(const char* name)
        : CTenant(name)
    {
    }

    ~CSensingTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;
};



#endif //C_SENSING_TENANT_H
