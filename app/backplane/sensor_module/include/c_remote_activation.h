#ifndef C_REMOTE_ACTIVATION_TENANT_H
#define C_REMOTE_ACTIVATION_TENANT_H

#include <f_core/os/c_tenant.h>
#include <zephyr/drivers/gpio.h>

class CRemoteActivationTenant : public CTenant {
public:
    explicit CRemoteActivationTenant(const char* name, const gpio_dt_spec &pin)
        : CTenant(name), pin(pin)
    {
    }

    ~CRemoteActivationTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    const gpio_dt_spec &pin;
};


#endif //C_REMOTE_ACTIVATION_TENANT_H
