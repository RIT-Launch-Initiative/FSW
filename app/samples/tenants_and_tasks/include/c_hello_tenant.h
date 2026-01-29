#pragma once

// F-Core Includes
#include <f_core/os/c_runnable_tenant.h>

/**
 * Print "Hello, " followed by a name.
 */
class CHelloTenant : public CRunnableTenant {
public:
    /**
     * Constructor
     * @param name The name to print after "Hello, ".
     */
    explicit CHelloTenant(const char *name);

    /**
     * See parent docs
     */
    void Run() override;
};


