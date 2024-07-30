/*
* Copyright (c) 2024 RIT Launch Initiative
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "c_sensing_tenant.h"
#include "c_broadcast_tenant.h"

int main() {
    static CBroadcastTenant broadcastTenant = CBroadcastTenant("Broadcast Tenant");

    for (int i = 0; i < 100; i++) {
        broadcastTenant.Run();
        k_msleep(1000);
    }

    return 0;
}

