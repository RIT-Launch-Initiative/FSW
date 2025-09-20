/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "f_core/os/c_tenant.h"

#ifndef C_RUNNABLE_TENANT_H
#define C_RUNNABLE_TENANT_H

/**
 * Tenant designed to be executed in a superloop or task periodically
 * F-Core tenants should ideally be non-blocking and return as quickly as possible
 * Application tenants can block, but should recognize other tenants that could be sharing the same thread context
 */
class CRunnableTenant : public CTenant {
public:
    /**
     * Constructor
     * @param name Name of the tenant
     */
    explicit CRunnableTenant(const char* name) : CTenant(name) {};

    /**
     * Destructor
     */
    virtual ~CRunnableTenant() = default;

    /**
     * Initialize the tenant
     */
    virtual void Startup() {};

    /**
     * Perform any other initializations after all tenants in the same task have been initialized
     */
    virtual void PostStartup() {};

    /**
     * Run the tenant. This function is allowed to block
     * Users should recognize other tenants that could be sharing the same thread context
     * Ideally, F-Core tenants should be non-blocking and return as quickly as possible
     */
    virtual void Run() = 0;
};

#endif //C_RUNNABLE_TENANT_H
