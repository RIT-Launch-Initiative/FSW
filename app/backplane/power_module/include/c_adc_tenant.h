#ifndef C_ADC_TENANT_H
#define C_ADC_TENANT_H

#include <n_autocoder_types.h>

#include <f_core/messaging/c_message_port.h>
#include <f_core/device/c_adc.h>
#include <f_core/os/c_tenant.h>

#include <zephyr/devicetree.h>

class CAdcTenant : public CTenant {
public:
    explicit CAdcTenant(const char* name, CMessagePort<int32_t> &dataToBroadcast, CMessagePort<int32_t> &dataToLog)
        : CTenant(name), dataToBroadcast(dataToBroadcast), dataToLog(dataToLog)
    {
    }

    ~CAdcTenant() override = default;

    void Startup() override;

    void PostStartup() override;

    void Run() override;

private:
    CAdc adc = CAdc(ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0));
    CMessagePort<int32_t> &dataToBroadcast;
    CMessagePort<int32_t> &dataToLog;
};

#endif //C_ADC_TENANT_H
