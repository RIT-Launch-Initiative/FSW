#ifndef C_FLIGHT_LOG_TENANT_H
#define C_FLIGHT_LOG_TENANT_H

#include <f_core/os/c_tenant.h>

class CFlightLogTenant : public CTenant {
public:
    explicit CFlightLogTenant(const char* name, ) : CTenant(name), filename(filename) {}
    ~CFlightLogTenant() override = default;

    void Startup() override;
    void PostStartup() override;
    void Run() override;

    void Cleanup() { dataLogger.close(); }
private:


};

#endif //C_FLIGHT_LOG_TENANT_H
