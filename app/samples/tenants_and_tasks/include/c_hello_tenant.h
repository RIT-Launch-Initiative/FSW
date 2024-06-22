#ifndef HELLOTENANT_H
#define HELLOTENANT_H

// F-Core Includes
#include <f_core/os/c_tenant.h>

class CHelloTenant : public CTenant {
public:
    explicit CHelloTenant(const char *name);

    void Run() override;
};

#endif //HELLOTENANT_H
