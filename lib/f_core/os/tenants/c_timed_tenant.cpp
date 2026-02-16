#include "f_core/os/tenants/c_timed_tenant.h"

#include "f_core/os/n_rtos.h"
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(CTimedTenant);

static void timerExpirationCallback(struct k_timer* timer) {
    auto* tenant = static_cast<CTimedTenant*>(k_timer_user_data_get(timer));
    if (tenant != nullptr) {
        tenant->Run();
    } else {
        LOG_ERR("Timed tenant callback with null tenant");
    }
}

CTimedTenant::CTimedTenant(const char* name, const uint32_t intervalMillis)
    : CTenant(name), timer(intervalMillis, timerExpirationCallback, nullptr) {
    timer.SetUserData(this);
}
