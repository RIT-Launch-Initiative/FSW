#ifndef HELLOTENANT_H
#define HELLOTENANT_H

// F-Core Includes
#include <f_core/os/c_tenant.h>

/**
 * Print "Hello, " followed by a name.
 */
class CReceiver : public CTenant {
public:
    /**
     * Constructor
     * @param name The name to print after "Hello, ".
     */
    explicit CReceiver(const char *name);

    /**
     * See parent docs
     */
    void Run() override;
};

#endif //HELLOTENANT_H
