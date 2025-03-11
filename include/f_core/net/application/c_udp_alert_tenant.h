#ifndef C_UDP_ALERT_TENANT_H
#define C_UDP_ALERT_TENANT_H

#include "f_core/os/c_tenant.h"

class CUdpAlertTenant : public CTenant {
public:
    explicit CUdpAlertTenant(const char* name)
        : CTenant(name) {}

private:

};



#endif //C_UDP_ALERT_TENANT_H
