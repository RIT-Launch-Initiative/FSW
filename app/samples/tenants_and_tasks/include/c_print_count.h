#ifndef PRINTCOUNT_H
#define PRINTCOUNT_H

// F-Core Includes
#include <f_core/os/c_tenant.h>

class CPrintCount : public CTenant {
public:
    explicit CPrintCount(const char* name, int* count);

    void Startup() override;

    void Run() override;

private:
    using CBase = CTenant;

    int* count;
};

#endif //PRINTCOUNT_H

