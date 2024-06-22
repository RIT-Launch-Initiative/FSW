#ifndef HELLOTENANT_H
#define HELLOTENANT_H

// F-Core Includes
#include <f_core/os/c_tenant.h>

class HelloTenant : public CTenant {
public:
    HelloTenant(const char* name, int priority, int stack_size, uint64_t time_slice);

    void Run() override;
};

#endif //HELLOTENANT_H
