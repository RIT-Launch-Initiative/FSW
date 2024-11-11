#ifndef C_RS485_TENANT_H
#define C_RS485_TENANT_H

#include <f_core/net/device/c_rs485.h>
#include <f_core/os/c_tenant.h>

#include <zephyr/drivers/gpio.h>


class CRs485Tenant : public CTenant {
public:
  CRs485Tenant(const char* name)
    : CTenant(name)
  {
  }

  void Startup() override;

  void PostStartup() override;

  void Run() override;

private:
  // CRs485 &rs485{DEVICE_DT_GET(DT_ALIAS(rs485_uart)), GPIO_DT_SPEC_GET(de_hack, leds)};;

};



#endif //C_RS485_TENANT_H
