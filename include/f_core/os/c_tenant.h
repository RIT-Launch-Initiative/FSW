/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

/**
 * Base class for all tenants - Ideally applications should not inherit from this class directly
 * A tenant is considered a software module meant to be ran periodically
 * The user can choose how they are executed whether in a task, callback, superloop...
 * See CRunnableTenant and CCallbackTenant for more specific types of tenants
 */
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
