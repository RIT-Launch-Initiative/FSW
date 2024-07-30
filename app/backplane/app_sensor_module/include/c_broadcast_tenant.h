#ifndef C_BROADCAST_TENANT_H
#define C_BROADCAST_TENANT_H

#include <f_core/os/c_tenant.h>

class CBroadcastTenant : public CTenant {
public:
    explicit CBroadcastTenant(const char* name)
        : CTenant(name)
    {
    }

    ~CBroadcastTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;
};



#endif //C_BROADCAST_TENANT_H
