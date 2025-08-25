#include "f_core/os/tenants/c_timed_tenant.h"

static void timerExpirationCallback(struct k_timer *timer) {

}

CTimedTenant::CTimedTenant(const char* name, const uint32_t intervalMillis)
    : CTenant(name), timer(intervalMillis, timerExpirationCallback) {}

void CTimedTenant::Startup() {

}