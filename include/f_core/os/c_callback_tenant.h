/*
* Copyright (c) 2025 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef C_CALLBACK_TENANT_H
#define C_CALLBACK_TENANT_H

#include "f_core/os/c_tenant.h"


class CCallbackTenant : public CTenant {
public:
    /**
     * Constructor
     * @param name Name of the tenant
     */
    explicit CCallbackTenant(const char *name) : CTenant(name) {};

    /**
     * Destructor
     */
    virtual ~CCallbackTenant() = default;

    /**
     * Function to execute on callback
     */
    virtual void Callback() = 0;
};

#endif //C_CALLBACK_TENANT_H
