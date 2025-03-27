#include <f_core/os/c_tenant.h>
#include "c_hello_tenant.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CHelloTenant);

CHelloTenant::CHelloTenant(const char* name) : CTenant(name) {

}

void CHelloTenant::Run() {
    LOG_INF("Hello, %s", name);
}

