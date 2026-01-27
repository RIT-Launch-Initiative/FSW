#pragma once

// F-Core Includes
#include <f_core/os/c_runnable_tenant.h>

/**
 * Increments and prints the current count for a given integer.
 */
class CPrintCount : public CRunnableTenant {
public:
    /**
     * Constructor.
     * @param name The name of the tenant.
     * @param count The integer to increment and print.
     */
    explicit CPrintCount(const char* name, int* count);

    /**
     * See parent docs
     */
    void Startup() override;

    /**
     * See parent docs
     */
    void Run() override;

private:
    using CBase = CRunnableTenant;

    int* count;
};
