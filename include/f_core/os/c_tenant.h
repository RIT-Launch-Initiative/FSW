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
     * Cleanup the tenant
     */
    virtual void Cleanup() {};

    /**
     * Get the name of the tenant
     * @return Name of the tenant
     */
    const char *GetName() const {
        return name;
    }
protected:
    const char *name;
};

#endif //C_TENANT_H
