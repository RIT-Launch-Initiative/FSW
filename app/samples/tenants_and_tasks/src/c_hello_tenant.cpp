#include <f_core/os/c_tenant.h>
#include "c_hello_tenant.h"
#include <zephyr/kernel.h>

CHelloTenant::CHelloTenant(const char *name) : CTenant(name) {

}

void CHelloTenant::Run() {
    printk("Hello, %s", name);
}

