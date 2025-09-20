/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "c_tenant.h"

#ifndef C_RUNNABLE_TENANT_H
#define C_RUNNABLE_TENANT_H

class CRunnableTenant : public CTenant {
public:
    /**
     * Constructor
     * @param name Name of the tenant
     */
    explicit CRunnableTenant(const char *name) : CTenant(name);

    /**
     * Destructor
     */
    virtual ~CRunnableTenant() = default;

    /**
     * Run the tenant
     */
    virtual void Run() = 0;
};

#endif //C_RUNNABLE_TENANT_H
