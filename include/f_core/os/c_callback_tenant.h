/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef C_CALLBACK_TENANT_H
#define C_CALLBACK_TENANT_H

#include "f_core/os/c_tenant.h"

/**
 * Tenant designed to be executed in a callback periodically
 */
class CCallbackTenant : public CTenant {
public:
    /**
     * Constructor
     * @param name Name of the tenant
     */
    explicit CCallbackTenant(const char *name) : CRunnableTenant(name) {};

    /**
     * Destructor
     */
    virtual ~CCallbackTenant() = default;

    /**
     * Sets up the tenant to be ran in a callback
     */
    virtual void Register() = 0;

    /**
     * Function to execute on callback. This function should not block
     */
    virtual void Callback() = 0;
};

#endif //C_CALLBACK_TENANT_H
