/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef C_TENANT_H
#define C_TENANT_H

class CTenant {
public:
    /**
     * Constructor
     * @param name Name of the tenant
     */
    explicit CTenant(const char *name);

    /**
     * Destructor
     */
    virtual ~CTenant() = default;

    /**
     * Initialize the tenant
     */
    virtual void Startup() {};

    /**
     * Perform any other initializations after all tenants in the same task have been initialized
     */
    virtual void PostStartup() {};

    /**
     * Cleanup the tenant
     */
    virtual void Cleanup() {};

    const char *GetName() const {
        return name;
    }
protected:
    const char *name;
};

#endif //C_TENANT_H
