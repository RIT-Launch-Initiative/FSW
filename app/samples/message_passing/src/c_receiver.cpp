#include <f_core/os/c_tenant.h>
#include "c_receiver.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CReceiver);

CReceiver::CReceiver(const char* name) : CTenant(name) {

}

void CReceiver::Run() {
    LOG_INF("Hello, %s", name);
}

