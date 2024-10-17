#ifndef C_RS485_TENANT_H
#define C_RS485_TENANT_H

#include <f_core/net/device/c_rs485.h>
#include <f_core/os/c_tenant.h>


class CRs485Tenant : public CTenant {
public:
  CRs485Tenant(const char* name)
    : CTenant(name)
  {
  }

  void Run() override;

private:

};



#endif //C_RS485_TENANT_H
