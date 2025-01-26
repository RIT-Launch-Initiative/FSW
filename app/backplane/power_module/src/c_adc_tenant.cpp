#include "c_adc_tenant.h"

#include <f_core/device/c_adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CAdcTenant);

void CAdcTenant::Startup() {
}

void CAdcTenant::PostStartup() {
}

void CAdcTenant::Run() {
    if (adc.UpdateAdcValue() < 0) {
        LOG_DBG("Skipping ADC read");
        return;
    }

    int32_t adcValue = adc.GetAdcValue();

    // TODO: Calculate using that math thing from the doc schematic

    dataToBroadcast.Send(adcValue, K_MSEC(5));
    dataToLog.Send(adcValue, K_MSEC(5));
}